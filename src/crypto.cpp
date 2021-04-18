#include "crypto.h"
#include "utils.h"

CryptUtilClass::CryptUtilClass()
{
    gcmaes256 = new GCM<AES256>();
}

int CryptUtilClass::encrypt(
    unsigned char *plaintext,
    const size_t plaintextLen,
    unsigned char *dstBuff,
    const size_t dstBuffLen,
    const unsigned long time)
{

    size_t totalLength = plaintextLen + sizeof(tag) + sizeof(iv);
    if (dstBuffLen < totalLength)
    {
        return -1;
    }

    // setup
    gcmaes256->clear();
    gcmaes256->setKey(key, gcmaes256->keySize());

    // use the time as the IV/Nonce
    memset(iv, 0, sizeof(iv)); // zero IV from previous usage
    writeUint32(time, iv, 0);  // copy time into IV
    gcmaes256->setIV(iv, sizeof(iv));

    // perform the encryption
    gcmaes256->encrypt(dstBuff, plaintext, plaintextLen);
    // compute the authentication tag
    gcmaes256->computeTag(tag, sizeof(tag));

    // append the authentication tag to dstBuff
    dstBuff += plaintextLen;           // move the pointer forward to the end of the cipher text
    memcpy(dstBuff, tag, sizeof(tag)); // and append computed tag

    // finally, append the IV/Nonce in clear text
    dstBuff += sizeof(tag);          // move the pointer to after the tag
    memcpy(dstBuff, iv, sizeof(iv)); // and append IV/Nonce

    return totalLength;
}

bool CryptUtilClass::decrypt(
    unsigned char ciphertextAndTagAndTime[],
    const size_t ciphertextAndTagAndTimeLength,
    unsigned char dstBuff[],
    const size_t dstBuffLength,
    const unsigned long time)
{
    gcmaes256->clear();

    size_t ciphertextSize = ciphertextAndTagAndTimeLength - sizeof(tag) - sizeof(iv);

    unsigned char *ptr = ciphertextAndTagAndTime;

    // extract the tag
    ptr += ciphertextSize;         // move pointer to after the ciphertext / start og tag
    memcpy(tag, ptr, sizeof(tag)); // copy out tag

    // extract the IV/Nonce - which contains the time
    ptr += sizeof(tag);          // move pointer to after the tag
    memcpy(iv, ptr, sizeof(iv)); // copy out IV

    // setup cipher
    gcmaes256->setKey(key, gcmaes256->keySize());

    // set IV/Nonce
    gcmaes256->setIV(iv, sizeof(iv));

    // decrypt the data (doesn't validate the tag)
    gcmaes256->decrypt(dstBuff, ciphertextAndTagAndTime, ciphertextSize);

    // check the tag
    if (!gcmaes256->checkTag(tag, sizeof(tag)))
    {
        Serial.println("Tag-validation failed");
        return false;
    }

    // good so far, extract the time from the IV/Nonce
    unsigned long receivedTime = toUInt(iv, 0);

    // compare with local time
    long delta = time - receivedTime;

    // allow 30 second clock-slew
    if (delta > 15 || delta < -15)
    {
        Serial.println("Time-validation failed");
        return false;
    }

    return true;
}

CryptUtilClass CryptUtil;
