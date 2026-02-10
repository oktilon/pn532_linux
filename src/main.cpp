#include <stdarg.h>
#include <stdio.h>

#include "lib/Pn532.h"
#include "config.h"
#include "main.h"

const char *logLevelHeaders[] = {
    "\033[1;31mERR\033[0m",  // LOG_LEVEL_ERROR     // q = quiet
    "\033[1;91mWRN\033[0m",  // LOG_LEVEL_WARNING   //   = default
    "\033[1;37mINF\033[0m",  // LOG_LEVEL_INFO      // v = verbose
    "\033[1;36mDBG\033[0m",  // LOG_LEVEL_DEBUG     // vvv = verbose++
    "\033[1;33mTRC\033[0m"   // LOG_LEVEL_TRACE     // vv = verbose+
};

const char *logLevelColor[] = {
    "\033[0;31m",  // LOG_LEVEL_ERROR   #BC1B27
    "\033[0;91m",  // LOG_LEVEL_WARNING #F15E42
    "\033[0;37m",  // LOG_LEVEL_INFO    #D0CFCC
    "\033[0;36m",  // LOG_LEVEL_DEBUG   #2AA1B3
    "\033[0;33m"   // LOG_LEVEL_TRACE   #A2734C
};

#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_WARNING   1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_DEBUG     3
#define LOG_LEVEL_TRACE     4


void logger (const char *file, int line, const char *func, int lvl, const char* fmt, ...) {
    int r;
    char *msg = NULL;
    int logIx = lvl < 0 ? 0 : (lvl > LOG_LEVEL_MAX ? LOG_LEVEL_MAX : lvl);

    // Format log message
    va_list arglist;
    va_start (arglist, fmt);
    r = vasprintf (&msg, fmt, arglist);
    va_end (arglist);

    printf("[%s] %s%s\033[0m [%s:%d] in %s\n", logLevelHeaders[logIx], logLevelColor[lvl], msg, file, line, func);
    fflush(stdout);
}

int main(int argc, char **argv) {
    int iStop = 0;
    uint8_t success;
    uint8_t currentblock;                     // Counter to keep track of which block we're on
    bool authenticated = false;               // Flag to indicate if the sector is authenticated
    uint8_t data[16];                         // Array to store block data during reads
    // Keyb on NDEF and Mifare Classic should be the same
    uint8_t keyUniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    printf("App %s version %s\n", PROJECT, VERSION);
    Pn532 nfc(0);
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        log_err ("Didn't find PN53x board");
        return 1;
    }

    log_inf("Found chip PN5%02hhX", (versiondata>>24) & 0xFF);
    log_inf("Firmware ver. %hhu.%hhu", (versiondata>>16) & 0xFF, (versiondata>>8) & 0xFF);

    nfc.setPassiveActivationRetries(0xFF);

    while (!iStop) {
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
        uint8_t uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

        if (success) {
            log_inf ("Found a card!");
            log_inf ("UID Length: %hhu bytes", uidLength);
            Pn532::PrintHex("UID Value", uid, uidLength);
            // Wait 1 second before continuing

            if (uidLength == 4) {
                // We probably have a Mifare Classic card ...
                log_inf ("Seems to be a Mifare Classic card (4 byte UID)");

                // Now we try to go through all 16 sectors (each having 4 blocks)
                // authenticating each sector, and then dumping the blocks
                for (currentblock = 0; currentblock < 64; currentblock++) {
                    // Check if this is a new block so that we can reauthenticate
                    if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

                    // If the sector hasn't been authenticated, do so first
                    if (!authenticated) {
                        // Starting of a new sector ... try to to authenticate
                        log_inf ("------------------------ Sector %hhu -------------------------", currentblock/4);
                        if (currentblock == 0) {
                            // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
                            // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
                            // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
                            success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyUniversal);
                        } else {
                            // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
                            // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
                            // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
                            success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyUniversal);
                        }
                        if (success) {
                            authenticated = true;
                        } else {
                            log_wrn ("Authentication error");
                        }
                    }
                    // If we're still not authenticated just skip the block
                    if (!authenticated) {
                        log_wrn ("Block %hhu unable to authenticate", currentblock);
                    } else {
                        // Authenticated ... we should be able to read the block now
                        // Dump the data into the 'data' array
                        success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
                        if (success) {
                            char blk[10] = {0};
                            sprintf(blk, "Block %hhu", currentblock);
                            Pn532::PrintHex(blk, data, 16);
                        } else {
                            // Oops ... something happened
                            log_wrn("Block %hhu unable to read this block", currentblock);
                        }
                    }
                }
            } else {
                log_wrn ("Ooops ... this doesn't seem to be a Mifare Classic card!");
            }
        } else {
            // PN532 probably timed out waiting for a card
            log_wrn ("Timed out waiting for a card");
        }
    }

    return 0;
}