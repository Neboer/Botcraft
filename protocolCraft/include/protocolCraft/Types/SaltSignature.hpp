#if PROTOCOL_VERSION > 758
#pragma once

#include <vector>

#include "protocolCraft/NetworkType.hpp"

namespace ProtocolCraft
{
    class SaltSignature : public NetworkType
    {
    public:
        virtual ~SaltSignature() override
        {

        }

        void SetSalt(const long long int salt_)
        {
            salt = salt_;
        }
        
        void SetSignature(const std::vector<unsigned char>& signature_)
        {
            signature = signature_;
        }
    

        const long long int GetSalt() const
        {
            return salt;
        }
        
        const std::vector<unsigned char>& GetSignature() const
        {
            return signature;
        }

    protected:
        virtual void ReadImpl(ReadIterator &iter, size_t &length) override
        {
            salt = ReadData<long long int>(iter, length);
            const int signature_size = ReadData<VarInt>(iter, length);
            signature = ReadByteArray(iter, length, signature_size);
        }

        virtual void WriteImpl(WriteContainer &container) const override
        {
            WriteData<long long int>(salt, container);
            WriteData<VarInt>(signature.size(), container);
            WriteByteArray(signature, container);
        }

        virtual const nlohmann::json SerializeImpl() const override
        {
            nlohmann::json output;

            output["salt"] = salt;
            output["signature"] = "Vector of " + std::to_string(signature.size()) + " unsigned char";


            return output;
        }

    private:
        long long int salt;
        std::vector<unsigned char> signature;
    };
} // ProtocolCraft
#endif
