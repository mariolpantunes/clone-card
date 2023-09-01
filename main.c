#include <stdlib.h>
#include <string.h>
#include <nfc/nfc.h>
#include <unistd.h>

/*

APDU commands for Magic Gen3 card.

cla  ins p1  p2  len
 90  F0  CC  CC  10 <block0>  - write block 0
 90  FB  CC  CC  07 <uid>     - change uid (independently of block0 data)
 90  FD  11  11  00           - lock permanently
*/

nfc_device *pnd = NULL;
nfc_context *context = NULL;

void prog_exit(char* msg, int code) {
    printf("%s", msg);
    if(pnd != NULL) {
        nfc_close(pnd);
        pnd = NULL;
    }
    
    if(context != NULL) {
		nfc_exit(context);
		context = NULL;
    }
    exit(code);
}

int main(int argc, const char *argv[])
{
	nfc_target nt_src;
	nfc_target nt_dst;

	nfc_init(&context);

	uint32_t cycles = 0;

	if (context == NULL) {
		prog_exit("ERROR: Unable to init libnfc (malloc)", EXIT_FAILURE);
	}

	int i = 0;
	while (i < 5) {
		pnd = nfc_open(context, NULL);
		if(pnd != NULL)
			break;
		sleep(0.5);
		i += 1;
	}

	if (pnd == NULL) {
		prog_exit("ERROR: Unable to open NFC device", EXIT_FAILURE);
	}
	
	if (nfc_initiator_init(pnd) < 0) {
		nfc_perror(pnd, "nfc_initiator_init");
		prog_exit("ERROR: NFC Initiator error", EXIT_FAILURE);
	}


	printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

	const nfc_modulation nmMifare = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	};

	printf("\nPlace source tag on reader...\n");
	while (nfc_initiator_list_passive_targets(pnd,  nmMifare, &nt_src, 1) <= 0);

	printf("Source UID len: %d Value: ", nt_src.nti.nai.szUidLen);
	for (int i = 0; i < nt_src.nti.nai.szUidLen; i++) {
		printf("%02x", nt_src.nti.nai.abtUid[i]);
	}
	printf("\n");

	if(nt_src.nti.nai.szUidLen > 7) {
		prog_exit("ERROR: Invalid UID Len", EXIT_FAILURE);
	}


	printf("\nRemove source tag from reader\n");
	while (nfc_initiator_list_passive_targets(pnd,  nmMifare, &nt_src, 1) > 0) {
		sleep(1);
	}


	printf("\nPlace destination tag on the reader\n");
	while (nfc_initiator_list_passive_targets(pnd,  nmMifare, &nt_dst, 1) <= 0);

	printf("Target UID len: %d Value: ", nt_dst.nti.nai.szUidLen);
	for (int i = 0; i < nt_dst.nti.nai.szUidLen; i++) {
		printf("%02x", nt_dst.nti.nai.abtUid[i]);
	}
	printf("\n");

	if( nt_src.nti.nai.szUidLen != nt_dst.nti.nai.szUidLen) {
		prog_exit("ERROR: Tag len mismatch", EXIT_FAILURE);
	}

	if(memcmp(nt_src.nti.nai.abtUid, nt_dst.nti.nai.abtUid, nt_src.nti.nai.szUidLen) == 0) {
		printf("WARNING: Tags UIDs are already the same\n");
	}

	// Use raw send/receive methods
	if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) { //for timed function.
		nfc_perror(pnd, "nfc_device_set_property_bool");
		prog_exit("ERROR: Could not set Frame property", EXIT_FAILURE);
	}


    printf("Writing UID to card...");
        
	uint8_t capdu[264]= {0};
	size_t capdulen;
	uint8_t rapdu[264]= {0};
	size_t rapdulen;

	//memcpy(capdu, "\x90\xFB\xCC\xCC\x07\x05\x83\xeb\x96\x33\x90\x00", 12);
	memcpy(capdu, "\x90\xFB\xCC\xCC", 4);
	capdu[4] = nt_src.nti.nai.szUidLen;
	memcpy(&capdu[5], nt_src.nti.nai.abtUid, nt_src.nti.nai.szUidLen);

	//hex value after: "\x90\xFB\xCC\xCC\x07 <UID> (in hex, use \x)
	// first hex part is APDU command.
	// to lock the UUID use this: '90  FD  11  11  00' APDU command
	// Write sector 0 (See top)

	capdulen = 5 + nt_src.nti.nai.szUidLen; //make sure size is correct after changing memcpy (also check for size at end)
	rapdulen = sizeof(rapdu);
    
	if (nfc_initiator_transceive_bytes_timed(pnd, capdu, capdulen, rapdu, rapdulen, &cycles) < 0) {
		prog_exit("FAIL\nERROR: Could not transmit data to card", EXIT_FAILURE);
	}

	if(rapdu[0] != 0x90 || rapdu[1] != 0x00) {
		printf("Done\nWARNING: Invalid response from card\n");
	}else {
	    printf("Done\n");
	}

	memset(&nt_dst, 0, sizeof(nfc_target));

	printf("\nRemove destination tag from reader\n");
	while (nfc_initiator_list_passive_targets(pnd,  nmMifare, &nt_dst, 1) > 0) {
		sleep(1);
	}

	printf("\nPlace destination tag on the reader\n");

	while (nfc_initiator_list_passive_targets(pnd,  nmMifare, &nt_dst, 1) <= 0);

	if(memcmp(nt_src.nti.nai.abtUid, nt_dst.nti.nai.abtUid, nt_src.nti.nai.szUidLen) == 0) {
		printf("Tags UID match! Clone succeeded\n");
	} else {
		printf("Tags UID do not match. Clone failed!\n");
	}

	prog_exit("Bye", EXIT_SUCCESS);
}
