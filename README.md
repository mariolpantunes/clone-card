====================== PCSC Mifare replacement ======================
Own implementation of the software used with the ACR122u reader software called PCSC Mifare from Piswords (a chinese seller).
Aliexpress sells these 7-byte changable UUID cards for Mifare Classic. Unfortanly it's not a normal magic card. 
It's like a magic magic magic card.
They implemented a custom command themself. I didn't have a proxmark3, or ACR122u reader. 
They also had software with uses the PC/SC API for their custom commands.
I only had a raspberry pi with the PN532 reader

=================================================
Unfortunately I would NOT recommend this card, it is read fine by my phone and the PN532 reader. 
But the reader where it was meant for, couldn't read the card. 
Don't buy it, if you already did. Try this gist and maybe you have better luck than me.
==================================================

I wrote my own implementation and port of the special Proxmark3 commands.
APDU commands:

cla  ins p1  p2  len
 90  F0  CC  CC  10 <block0>  - write block 0
 90  FB  CC  CC  07 <uid>     - change uid (independently of block0 data)
 90  FD  11  11  00           - lock permanently
 
 Proxmark uses these commands:
 
 # change just UID:
hf 14a raw -s -c  -t 2000  90FBCCCC07 11223344556677
# read block0:
hf 14a raw -s -c 3000
# write block0:
hf 14a raw -s -c  -t 2000  90F0CCCC10 041219c3219316984200e32000000000
# lock (uid/block0?) forever:
hf 14a raw -s -c 90FD111100

=================== Specs ==========================
Specs for the Mifare card are:

It seems the length byte gets ignored anyway.

Note: it seems some cards only accept the "change UID" command.

It accepts direct read of block0 (and only block0) without prior auth.

Writing to block 0 has some side-effects:

It changes also the UID. Changing the UID does not change block 0.
ATQA and SAK bytes are automatically replaced by fixed values.
On 4-byte UID cards, BCC byte is automatically corrected.

Source: https://github.com/RfidResearchGroup/proxmark3/blob/master/doc/magic_cards_notes.md#mifare-classic-apdu-aka-gen3
 
=================== Source code of APDU command for RASPBERRY PI AND PN532 =====================

Change your UID on this line:    memcpy(capdu, "\x90\xFB\xCC\xCC\x07\x06\xd3\xd1\x4a\x7b\x17\x90", 12);
You can decide to lock the UID with the command in the header...
After the \x07 byte

Compile like this: gcc magic_gen3.c -o magic_gen3 -lnfc
If you get linker errors, install lib-nfc dev: sudo apt-get install libnfc-dev (i think)

Run it like this: ./magic_gen3 
Make sure ofc your libnfc device is working...

========================== C- code ===========================