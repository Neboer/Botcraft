#pragma once

#include "protocolCraft/BaseMessage.hpp"

#if PROTOCOL_VERSION > 758
#include "protocolCraft/Types/SaltSignature.hpp"
#endif

namespace ProtocolCraft
{
    class ServerboundKeyPacket : public BaseMessage<ServerboundKeyPacket>
    {
    public:
        virtual const int GetId() const override
        {
            return 0x01;
        }

        virtual const std::string GetName() const override
        {
            return "Key";
        }

        virtual ~ServerboundKeyPacket() override
        {

        }

        void SetKeyBytes(const std::vector<unsigned char> &key_bytes_)
        {
            key_bytes = key_bytes_;
        }

        void SetNonce(const std::vector<unsigned char>& nonce_)
        {
            nonce = nonce_;
        }

#if PROTOCOL_VERSION > 758
        void SetHasSignature(const SaltSignature& salt_signature_)
        {
            salt_signature = salt_signature_;
        }
#endif

        const std::vector<unsigned char>& GetKeyBytes() const
        {
            return key_bytes;
        }

        const std::vector<unsigned char>& GetNonce() const
        {
            return nonce;
        }

#if PROTOCOL_VERSION > 758
        const SaltSignature& GetSaltSignature() const
        {
            return salt_signature;
        }
#endif

    protected:
        virtual void ReadImpl(ReadIterator &iter, size_t &length) override
        {
            const int key_bytes_length = ReadData<VarInt>(iter, length);
            key_bytes = ReadByteArray(iter, length, key_bytes_length);
#if PROTOCOL_VERSION > 758
            const bool has_nonce = ReadData<bool>(iter, length);
            if (has_nonce)
            {
                const int nonce_length = ReadData<VarInt>(iter, length);
                nonce = ReadByteArray(iter, length, nonce_length);
            }
            else
            {
                salt_signature.Read(iter, length);
            }
#else
            const int nonce_length = ReadData<VarInt>(iter, length);
            nonce = ReadByteArray(iter, length, nonce_length);
#endif
        }

        virtual void WriteImpl(WriteContainer &container) const override
        {
            WriteData<VarInt>(key_bytes.size(), container);
            WriteByteArray(key_bytes, container);
#if PROTOCOL_VERSION > 758
            WriteData<bool>(salt_signature.GetSignature().empty(), container);
            if (salt_signature.GetSignature().empty())
            {
                WriteData<VarInt>(nonce.size(), container);
                WriteByteArray(nonce, container);
            }
            else
            {
                salt_signature.Write(container);
            }
#else
            WriteData<VarInt>(nonce.size(), container);
            WriteByteArray(nonce, container);
#endif
        }

        virtual const nlohmann::json SerializeImpl() const override
        {
            nlohmann::json output;

            output["key_bytes"] = "vector of " + std::to_string(key_bytes.size()) + " unsigned char";
#if PROTOCOL_VERSION > 758
            if (salt_signature.GetSignature().empty())
            {
                output["nonce"] = "vector of " + std::to_string(nonce.size()) + " unsigned char";
            }
            else
            {
                output["salt_signature"] = salt_signature.Serialize();
            }
#else
            output["nonce"] = "vector of " + std::to_string(nonce.size()) + " unsigned char";
#endif

            return output;
        }

    private:
        std::vector<unsigned char> key_bytes;
        std::vector<unsigned char> nonce;
#if PROTOCOL_VERSION > 758
        SaltSignature salt_signature;
#endif
    };
} // Botcraft