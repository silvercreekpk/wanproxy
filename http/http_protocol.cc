#include <common/buffer.h>

#include <http/http_protocol.h>

namespace {
	static bool decode_nibble(uint8_t *out, uint8_t ch)
	{
		if (ch >= '0' && ch <= '9') {
			*out |= 0x00 + (ch - '0');
			return (true);
		}
		if (ch >= 'a' && ch <= 'f') {
			*out |= 0x0a + (ch - 'a');
			return (true);
		}
		if (ch >= 'A' && ch <= 'F') {
			*out |= 0x0a + (ch - 'A');
			return (true);
		}
		return (false);
	}

	static bool extract_line(Buffer *line, Buffer *input)
	{
		if (input->empty()) {
			ERROR("/http/protocol/extract_line") << "Empty buffer.";
			return (false);
		}

		unsigned pos;
		uint8_t found;
		if (!input->find_any("\r\n", &pos, &found)) {
			/*
			 * XXX
			 * This should be DEBUG once we can indicate that
			 * this is a recoverable error.
			 */
			ERROR("/http/protocol/extract_line") << "Incomplete line.";
			return (false);
		}

		/*
		 * XXX
		 * We should pick line ending from the start line and require it to
		 * be consistent for remaining lines, rather than using find_any over
		 * and over, which is non-trivial.  Handling of the start line can be
		 * quite easily and cheaply before the loop.
		 */
		switch (found) {
		case '\r':
			/* CRLF line endings.  */
			ASSERT(input->length() > pos);
			if (input->length() == pos + 1) {
				/*
				 * XXX
				 * This should be DEBUG once we can indicate that
				 * this is a recoverable error.
				 */
				ERROR("/http/protocol/extract_line") << "Carriage return at end of buffer, need following line feed.";
				return (false);
			}
			if (pos != 0)
				input->moveout(line, pos);
			input->skip(1);
			if (input->peek() != '\n') {
				ERROR("/http/protocol/extract_line") << "Carriage return not followed by line feed.";
				return (false);
			}
			input->skip(1);
			break;
		case '\n':
			/* Unix line endings.  */
			if (pos != 0)
				input->moveout(line, pos);
			input->skip(1);
			break;
		default:
			NOTREACHED();
		}

		return (true);
	}
}

bool
HTTPProtocol::Message::decode(Buffer *input)
{
	if (start_line_.empty()) {
		start_line_.clear();
		headers_.clear();
		body_.clear();
	}

	Buffer line;
	if (!extract_line(&line, input)) {
		ERROR("/http/protocol/message") << "Could not get start line.";
		return (false);
	}
	if (line.empty()) {
		ERROR("/http/protocol/message") << "Premature end of headers.";
		return (false);
	}
	start_line_ = line;

	/*
	 * There are two kinds of request line.  The first has two
	 * words, the second has three.  Anything else is malformed.
	 *
	 * The first kind is HTTP/0.9.  The second kind can be
	 * anything, especially HTTP/1.0 and HTTP/1.1.
	 */
	std::vector<Buffer> words = line.split(' ', false);
	if (words.empty()) {
		ERROR("/http/protocol/message") << "Empty start line.";
		return (false);
	}

	if (words.size() == 2) {
		/*
		 * HTTP/0.9.  This is all we should get from the client.
		 */
		return (true);
	}

	if (words.size() != 3) {
		ERROR("/http/protocol/message") << "Too many request parameters.";
		return (false);
	}

	/*
	 * HTTP/1.0 or HTTP/1.1.  Get headers.
	 */
	std::string last_header;
	for (;;) {
		ASSERT(line.empty());
		if (!extract_line(&line, input)) {
			ERROR("/http/protocol/message") << "Could not extract line for headers.";
			return (false);
		}

		/*
		 * Process end of headers!
		 */
		if (line.empty()) {
			/*
			 * XXX
			 * Use Content-Length, Transfer-Encoding, etc.
			 */
			if (!input->empty()) {
				body_.append(input);
				input->clear();
			}
			return (true);
		}

		/*
		 * Process header.
		 */
		if (line.peek() == ' ') { /* XXX isspace? */
			/*
			 * Fold headers per RFC822.
			 * 
			 * XXX Always forget how to handle leading whitespace.
			 */
			if (last_header == "") {
				ERROR("/http/protocol/message") << "Folded header sent before any others.";
				return (false);
			}

			headers_[last_header].back().append(line);
			line.clear();
			continue;
		}

		unsigned pos;
		if (!line.find(':', &pos)) {
			ERROR("/http/protocol/message") << "Empty header name.";
			return (false);
		}

		Buffer key;
		line.moveout(&key, pos);
		line.skip(1);

		Buffer value;
		while (!line.empty() && line.peek() == ' ')
			line.skip(1);
		value = line;

		std::string header;
		key.extract(header);

		headers_[header].push_back(value);
		last_header = header;

		line.clear();
	}
}

bool
HTTPProtocol::DecodeURI(Buffer *encoded, Buffer *decoded)
{
	if (encoded->empty())
		return (true);

	for (;;) {
		unsigned pos;
		if (!encoded->find('%', &pos)) {
			decoded->append(encoded);
			encoded->clear();
			return (true);
		}
		if (pos != 0)
			encoded->moveout(decoded, pos);
		if (encoded->length() < 3)
			return (false);
		uint8_t vis[2];
		encoded->copyout(vis, 1, 2);
		uint8_t byte = 0x00;
		if (!decode_nibble(&byte, vis[0]))
			return (false);
		byte <<= 4;
		if (!decode_nibble(&byte, vis[1]))
			return (false);
		decoded->append(byte);
		encoded->skip(3);
	}
}
