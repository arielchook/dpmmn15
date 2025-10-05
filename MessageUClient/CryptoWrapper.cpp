// main.cpp
// author: Ariel Cohen ID: 329599187

#include "CryptoWrapper.h"
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <stdexcept>

void CryptoWrapper::generateRsaKeys(CryptoPP::RSA::PrivateKey& privateKey, CryptoPP::RSA::PublicKey& publicKey) {
    CryptoPP::AutoSeededRandomPool rng;
    privateKey.Initialize(rng, 1024);
    publicKey = CryptoPP::RSA::PublicKey(privateKey);
}

std::string CryptoWrapper::privateKeyToBase64(const CryptoPP::RSA::PrivateKey& key) {
    std::string encoded;
    CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(encoded), false);
    key.DEREncode(encoder);
    encoder.MessageEnd();
    return encoded;
}

void CryptoWrapper::base64ToPrivateKey(const std::string& b64, CryptoPP::RSA::PrivateKey& key) {
    CryptoPP::StringSource source(b64, true, new CryptoPP::Base64Decoder);
    key.BERDecode(source);
}

std::vector<uint8_t> CryptoWrapper::publicKeyToBytes(const CryptoPP::RSA::PublicKey& key) {
    std::vector<uint8_t> bytes;
    CryptoPP::VectorSink sink(bytes);
    key.DEREncode(sink);
    return bytes;
}

void CryptoWrapper::bytesToPublicKey(const std::vector<uint8_t>& bytes, CryptoPP::RSA::PublicKey& key) {
    CryptoPP::VectorSource source(bytes, true);
    key.BERDecode(source);
}

std::vector<uint8_t> CryptoWrapper::rsaEncrypt(const CryptoPP::RSA::PublicKey& key, const std::vector<uint8_t>& plaintext) {
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSAES_PKCS1v15_Encryptor e(key);
    std::vector<uint8_t> ciphertext;
    CryptoPP::VectorSource ss(plaintext, true, new CryptoPP::PK_EncryptorFilter(rng, e, new CryptoPP::VectorSink(ciphertext)));
    return ciphertext;
}

std::vector<uint8_t> CryptoWrapper::rsaDecrypt(const CryptoPP::RSA::PrivateKey& key, const std::vector<uint8_t>& ciphertext) {
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSAES_PKCS1v15_Decryptor d(key);
    std::vector<uint8_t> plaintext;
    CryptoPP::VectorSource ss(ciphertext, true, new CryptoPP::PK_DecryptorFilter(rng, d, new CryptoPP::VectorSink(plaintext)));
    return plaintext;
}

std::vector<uint8_t> CryptoWrapper::aesEncrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plaintext) {
    // IV is all zeros as per specification
    std::vector<uint8_t> iv(CryptoPP::AES::BLOCKSIZE, 0);
    std::vector<uint8_t> ciphertext;
    try {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption e;
        e.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
        CryptoPP::VectorSource ss(plaintext, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::VectorSink(ciphertext)));
    }
    catch (const CryptoPP::Exception& e) {
        throw std::runtime_error(e.what());
    }
    return ciphertext;
}

std::vector<uint8_t> CryptoWrapper::aesDecrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& ciphertext) {
    // IV is all zeros as per specification
    std::vector<uint8_t> iv(CryptoPP::AES::BLOCKSIZE, 0);
    std::vector<uint8_t> plaintext;
    try {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
        d.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
        CryptoPP::VectorSource ss(ciphertext, true, new CryptoPP::StreamTransformationFilter(d, new CryptoPP::VectorSink(plaintext)));
    }
    catch (const CryptoPP::Exception& e) {
        throw std::runtime_error(e.what());
    }
    return plaintext;
}

std::vector<uint8_t> CryptoWrapper::generateAesKey() {
    CryptoPP::AutoSeededRandomPool rng;
    std::vector<uint8_t> key(CryptoPP::AES::DEFAULT_KEYLENGTH);
    rng.GenerateBlock(key.data(), key.size());
    return key;
}