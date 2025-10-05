// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include <string>
#include <vector>
#include <cryptopp/rsa.h>

class CryptoWrapper {
public:
    // Generates a 1024-bit RSA key pair
    static void generateRsaKeys(CryptoPP::RSA::PrivateKey& privateKey, CryptoPP::RSA::PublicKey& publicKey);

    // Converts keys to/from storable formats
    static std::string privateKeyToBase64(const CryptoPP::RSA::PrivateKey& key);
    static void base64ToPrivateKey(const std::string& b64, CryptoPP::RSA::PrivateKey& key);
    static std::vector<uint8_t> publicKeyToBytes(const CryptoPP::RSA::PublicKey& key);
    static void bytesToPublicKey(const std::vector<uint8_t>& bytes, CryptoPP::RSA::PublicKey& key);

    // Cryptographic operations
    static std::vector<uint8_t> rsaEncrypt(const CryptoPP::RSA::PublicKey& key, const std::vector<uint8_t>& plaintext);
    static std::vector<uint8_t> rsaDecrypt(const CryptoPP::RSA::PrivateKey& key, const std::vector<uint8_t>& ciphertext);
    static std::vector<uint8_t> aesEncrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plaintext);
    static std::vector<uint8_t> aesDecrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& ciphertext);

    // Key generation
    static std::vector<uint8_t> generateAesKey();
}; 
