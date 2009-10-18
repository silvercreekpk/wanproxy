#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;
#if defined(XCODEC_PIPES)
class XCodecEncoderPipe;
#endif

class XCodecEncoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
#if defined(XCODEC_PIPES)
	XCodecEncoderPipe *pipe_;
#endif
	Buffer queued_;

public:
#if defined(XCODEC_PIPES)
	XCodecEncoder(XCodec *, XCodecEncoderPipe *);
#else
	XCodecEncoder(XCodec *);
#endif
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);

	void encode_ask(uint64_t);
	void encode_learn(BufferSegment *);
private:
	void encode_declaration(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment **);
	void encode_escape(Buffer *, Buffer *, unsigned);
	bool encode_reference(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment *);
};

#endif /* !XCODEC_ENCODER_H */
