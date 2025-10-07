// main.cpp
// author: Ariel Cohen ID: 329599187

#pragma once
#include <string>
#include <vector>
#include <cryptopp/rsa.h>

/**
 * @brief A wrapper class for cryptographic operations using Crypto++.
 */
class CryptoWrapper {
public:
    /**
     * @brief Generates a 1024-bit RSA key pair.
     * @param privateKey The generated private key.
     * @param publicKey The generated public key.
     */
    static void generateRsaKeys(CryptoPP::RSA::PrivateKey& privateKey, CryptoPP::RSA::PublicKey& publicKey);

    /**
     * @brief Converts a private key to a Base64 encoded string.
     * @param key The private key to convert.
     * @return The Base64 encoded private key.
     */
    static std::string privateKeyToBase64(const CryptoPP::RSA::PrivateKey& key);

    /**
     * @brief Converts a Base64 encoded string to a private key.
     * @param b64 The Base64 encoded private key.
     * @param key The generated private key.
     */
    static void base64ToPrivateKey(const std::string& b64, CryptoPP::RSA::PrivateKey& key);

    /**
     * @brief Converts a public key to a byte vector.
     * @param key The public key to convert.
     * @return The byte vector representing the public key.
     */
    static std::vector<uint8_t> publicKeyToBytes(const CryptoPP::RSA::PublicKey& key);

    /**
     * @brief Converts a byte vector to a public key.
     * @param bytes The byte vector representing the public key.
     * @param key The generated public key.
     */
    static void bytesToPublicKey(const std::vector<uint8_t>& bytes, CryptoPP::RSA::PublicKey& key);

    /**
     * @brief Encrypts a plaintext using RSA.
     * @param key The public key to use for encryption.
     * @param plaintext The plaintext to encrypt.
     * @return The ciphertext.
     */
    static std::vector<uint8_t> rsaEncrypt(const CryptoPP::RSA::PublicKey& key, const std::vector<uint8_t>& plaintext);

    /**
     * @brief Decrypts a ciphertext using RSA.
     * @param key The private key to use for decryption.
     * @param ciphertext The ciphertext to decrypt.
     * @return The plaintext.
     */
    static std::vector<uint8_t> rsaDecrypt(const CryptoPP::RSA::PrivateKey& key, const std::vector<uint8_t>& ciphertext);

    /**
     * @brief Encrypts a plaintext using AES.
     * @param key The AES key to use for encryption.
     * @param plaintext The plaintext to encrypt.
     * @return The ciphertext.
     */
    static std::vector<uint8_t> aesEncrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plaintext);

    /**
     * @brief Decrypts a ciphertext using AES.
     * @param key The AES key to use for decryption.
     * @param ciphertext The ciphertext to decrypt.
     * @return The plaintext.
     */
    static std::vector<uint8_t> aesDecrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& ciphertext);

    /**
     * @brief Generates a 128-bit AES key.
     * @return The generated AES key.
     */
    static std::vector<uint8_t> generateAesKey();
}; 
