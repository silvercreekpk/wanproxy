#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <common/buffer.h>

#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>

namespace {
	class RSAServerHostKey : public SSH::ServerHostKey {
		LogHandle log_;
		RSA *rsa_;

		RSAServerHostKey(RSA *rsa)
		: SSH::ServerHostKey("ssh-rsa"),
		  log_("/ssh/serverhostkey/rsa"),
		  rsa_(rsa)
		{ }
	public:

		~RSAServerHostKey()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}

		void encode_public_key(Buffer *out) const
		{
			Buffer tag("ssh-rsa");

			SSH::String::encode(out, &tag);
			SSH::MPInt::encode(out, rsa_->e);
			SSH::MPInt::encode(out, rsa_->n);
		}

		static RSAServerHostKey *open(FILE *file)
		{
			RSA *rsa;

			rsa = PEM_read_RSAPrivateKey(file, NULL, NULL, NULL);
			if (rsa == NULL)
				return (NULL);

			return (new RSAServerHostKey(rsa));
		}
	};
}

SSH::ServerHostKey *
SSH::ServerHostKey::server(const std::string& keyfile)
{
	SSH::ServerHostKey *key;
	FILE *file;

	file = fopen(keyfile.c_str(), "r");
	if (file == NULL)
		return (NULL);
	
	key = RSAServerHostKey::open(file);
	if (key == NULL) {
		fclose(file);
		return (NULL);
	}

	fclose(file);

	return (key);
}
