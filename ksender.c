#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

/* Structura folosita pentru a simula un pachet MINI-KERMIT */

typedef struct {
	char SOH;
	char LEN;
	char SEQ;
	char TYPE;
	char DATA[250];
	unsigned short CHECK;
	char MARK;
} __attribute__ ((__packed__)) package;


/* Functie folosita pentru a trimite un pachet si pentru a astepta raspunsul la acest pachet. In caz ca
raspunsul este NULL de 3 ori, se va inchide transmisiunea */

int sendPackage3Times(msg t, msg **y){
	
	send_message(&t);

    *y = receive_message_timeout(5000);
	
    int resend_count = 0;
    while(*y == NULL){
    	send_message(&t);
    	*y = receive_message_timeout(5000);
    	resend_count++;
    	printf("Se reincearca transmiterea pachetului. Count: %d\n", resend_count);
    	if(resend_count == 3){
    		printf("Inchidere transmisiune!\n");
    		return -1;
    	}
    }

    return 1;
}

/* Functie folosita pentru a muta toate datele dintr-un pachet in payload in cazul in care se poate
aplica functia strlen asupra campului de date din pachet (ex. File-Header)
 */

void package2payloadData(package pack, char *payload){
	char *copy = payload;
	memcpy(copy, &pack.SOH, sizeof(pack.SOH));
    memcpy(copy += sizeof(pack.SOH), &pack.LEN, sizeof(pack.LEN));
    memcpy(copy += sizeof(pack.LEN), &pack.SEQ, sizeof(pack.SEQ));
    memcpy(copy += sizeof(pack.SEQ), &pack.TYPE, sizeof(pack.TYPE));
    memcpy(copy += sizeof(pack.TYPE), &pack.DATA, strlen(pack.DATA));
    memcpy(copy += strlen(pack.DATA), &pack.CHECK, sizeof(pack.CHECK));
    memcpy(copy += sizeof(pack.CHECK), &pack.MARK, sizeof(pack.MARK));
}

/* Functie folosita pentru a muta toate datele dintr-un pachet in payload in cazul in care pachetul
este de tip 'D' si primeste ca si parametru, pe langa payload si pachet, numarul de octeti cititi din 
fisierul de input */

void package2payloadDataBytes(package pack, char *payload, size_t bytes_read){
	char *copy = payload;
	memcpy(copy, &pack.SOH, sizeof(pack.SOH));
    memcpy(copy += sizeof(pack.SOH), &pack.LEN, sizeof(pack.LEN));
    memcpy(copy += sizeof(pack.LEN), &pack.SEQ, sizeof(pack.SEQ));
    memcpy(copy += sizeof(pack.SEQ), &pack.TYPE, sizeof(pack.TYPE));
    memcpy(copy += sizeof(pack.TYPE), &pack.DATA, bytes_read);
    memcpy(copy += bytes_read, &pack.CHECK, sizeof(pack.CHECK));
    memcpy(copy += sizeof(pack.CHECK), &pack.MARK, sizeof(pack.MARK));
}

/* Functie folosita pentru a muta toate datele dintr-un pachet in payload in cazul in care pachetul
are campul de date vid
 */

void package2payloadNullData(package pack, char *payload){
	char *copy = payload;
	memcpy(copy, &pack.SOH, sizeof(pack.SOH));
    memcpy(copy += sizeof(pack.SOH), &pack.LEN, sizeof(pack.LEN));
    memcpy(copy += sizeof(pack.LEN), &pack.SEQ, sizeof(pack.SEQ));
    memcpy(copy += sizeof(pack.SEQ), &pack.TYPE, sizeof(pack.TYPE));
    memcpy(copy += sizeof(pack.TYPE), &pack.CHECK, sizeof(pack.CHECK));
    memcpy(copy += sizeof(pack.CHECK), &pack.MARK, sizeof(pack.MARK));
}

/* Functie folosita pentru a initializa si crea un pachet de tip 'F' */

package init_F(char filename[20]){
	package F_Package;
	char EOL = 0x0D;
	char aux_string_crc[50];

	F_Package.SOH = 0x01;
    F_Package.LEN = 0x05 + strlen(filename);
    F_Package.SEQ = 0x00;
    F_Package.TYPE = 'F';
    strcpy(F_Package.DATA, filename);

    aux_string_crc[0] = F_Package.SOH;
    aux_string_crc[1] = F_Package.LEN;
    aux_string_crc[2] = F_Package.SEQ;
    aux_string_crc[3] = F_Package.TYPE;

    char *p = aux_string_crc;
    memcpy(p += 4, &F_Package.DATA, strlen(F_Package.DATA));

    F_Package.CHECK = crc16_ccitt(aux_string_crc, 4 + strlen(F_Package.DATA));
    F_Package.MARK = EOL;

    return F_Package;
}

/* Functie folosita pentru a initializa si crea un pachet de tip 'Z' */

package init_Z(){
	package Z_Package;
	char EOL = 0x0D;
	char aux_string_crc[50];

	Z_Package.SOH = 0x01;
    Z_Package.LEN = 0x05;
    Z_Package.SEQ = 0x00;
    Z_Package.TYPE = 'Z';

    aux_string_crc[0] = Z_Package.SOH;
    aux_string_crc[1] = Z_Package.LEN;
    aux_string_crc[2] = Z_Package.SEQ;
    aux_string_crc[3] = Z_Package.TYPE;

    Z_Package.CHECK = crc16_ccitt(aux_string_crc, 4);
    Z_Package.MARK = EOL;

    return Z_Package;
}

/* Functie folosita pentru a initializa si crea un pachet de tip 'B' */

package init_B(){
	package B_Package;
	char EOL = 0x0D;
	char aux_string_crc[50];

	B_Package.SOH = 0x01;
    B_Package.LEN = 0x05;
    B_Package.SEQ = 0x00;
    B_Package.TYPE = 'B';

    aux_string_crc[0] = B_Package.SOH;
    aux_string_crc[1] = B_Package.LEN;
    aux_string_crc[2] = B_Package.SEQ;
    aux_string_crc[3] = B_Package.TYPE;

    B_Package.CHECK = crc16_ccitt(aux_string_crc, 4);
    B_Package.MARK = EOL;

    return B_Package;
}

/* Functie folosita pentru a initializa si crea un pachet de tip 'D' */

package init_D(unsigned char *read_buffer, size_t bytes_read){
	package D_Package;
	char EOL = 0x0D;
	char aux_string_crc[255];

	D_Package.SOH = 0x01;
    D_Package.LEN = 0x05 + bytes_read;
    D_Package.SEQ = 0x00;
    D_Package.TYPE = 'D';
    memcpy(D_Package.DATA, read_buffer, bytes_read);

    aux_string_crc[0] = D_Package.SOH;
    aux_string_crc[1] = D_Package.LEN;
    aux_string_crc[2] = D_Package.SEQ;
    aux_string_crc[3] = D_Package.TYPE;

    char *p = aux_string_crc;
    memcpy(p += 4, &D_Package.DATA, bytes_read);

    D_Package.CHECK = crc16_ccitt(aux_string_crc, 4 + bytes_read);
    D_Package.MARK = EOL;

    return D_Package;
}


/* Functie folosita pentru a recalcula CRC-ul unui pachet in cazul in care trebuie incrementat numarul 
de secventa din cadrul pachetului */

void updateCRC(char *payload){
	unsigned short aux_crc;
	char auxiliar[255];
	unsigned char crc[2];
	auxiliar[0] = payload[0];
	auxiliar[1] = payload[1];
	auxiliar[2] = payload[2];
	auxiliar[3] = payload[3];
	char *p = auxiliar;
	char *r = payload;
	r += 4;
	memcpy(p += 4, r, payload[1] - 5);

	aux_crc = crc16_ccitt(auxiliar, payload[1] - 1);

	crc[0] = aux_crc & 0xff;
    crc[1] = (aux_crc >> 8) & 0xff;
    payload[payload[1] - 1] = crc[0];
    payload[(int)payload[1]] = crc[1];
}

/* Functie folosita pentru a recalcula CRC-ul unui pachet in cazul in care trebuie incrementat numarul 
de secventa din cadrul pachetului, daca pachetul este unul de tip 'D' */

void updateCRCFiles(char *payload){
	unsigned short aux_crc;
	char auxiliar[255];
	unsigned char crc[2];
	auxiliar[0] = payload[0];
	auxiliar[1] = payload[1];
	auxiliar[2] = payload[2];
	auxiliar[3] = payload[3];
	char *p = auxiliar;
	char *r = payload;
	r += 4;
	int length = (unsigned char) payload[1];
	memcpy(p += 4, r, length - 5);

	aux_crc = crc16_ccitt(auxiliar, length - 1);

	crc[0] = aux_crc & 0xff;
    crc[1] = (aux_crc >> 8) & 0xff;
    payload[length - 1] = crc[0];
    payload[length] = crc[1];
}


int main(int argc, char** argv) {

    msg t;
   	msg *y;
   	FILE *fp;
   	package F, D, Z, B;
   	unsigned char read_buffer[250];
    init(HOST, PORT);

    char MAXL = 250, TIME = 5, NPAD = 0x00, PADC = 0x00, EOL = 0x0D;
    char QCTL = 0x00, QBIN = 0x00, CHKT = 0x00, REPT = 0x00, CAPA = 0x00, R = 0x00;

    // Initializare pachet de tip Send-Init
    package S_Package;
    S_Package.SOH = 0x01;
    S_Package.LEN = 0x10;
    S_Package.SEQ = 0x00;
    S_Package.TYPE = 'S';
    S_Package.DATA[0] = MAXL;
    S_Package.DATA[1] = TIME;
    S_Package.DATA[2] = NPAD;
    S_Package.DATA[3] = PADC;
    S_Package.DATA[4] = EOL;
    S_Package.DATA[5] = QCTL;
    S_Package.DATA[6] = QBIN;
    S_Package.DATA[7] = CHKT;
    S_Package.DATA[8] = REPT;
    S_Package.DATA[9] = CAPA;
    S_Package.DATA[10] = R;
    S_Package.MARK = EOL;

    //Copiere packet de tip Send-Init in payload
    char *copy = t.payload;
    memcpy(copy, &S_Package.SOH, sizeof(S_Package.SOH));
    memcpy(copy += sizeof(S_Package.SOH), &S_Package.LEN, sizeof(S_Package.LEN));
    memcpy(copy += sizeof(S_Package.LEN), &S_Package.SEQ, sizeof(S_Package.SEQ));
    memcpy(copy += sizeof(S_Package.SEQ), &S_Package.TYPE, sizeof(S_Package.TYPE));
    memcpy(copy += sizeof(S_Package.TYPE), &S_Package.DATA, 11);
	S_Package.CHECK = crc16_ccitt(t.payload, 15);
    memcpy(copy += 11, &S_Package.CHECK, sizeof(S_Package.CHECK));
    memcpy(copy += sizeof(S_Package.CHECK), &S_Package.MARK, sizeof(S_Package.MARK));

	t.len = (int) (t.payload[1] + 2);
	

	/* Trimiterea pachetului de tip Send-Init */

    if(sendPackage3Times(t, &y) == -1)
    	return -1;
 
    printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

    /* Cat timp se primeste NACK, se incrementeaza numarul de secventa al pachetului, se recalculeaza
    CRC-ul si se incearca retrimiterea sa */

    while(y->payload[3] == 'N'){
    	t.payload[2] = y->payload[2] + 1;
    	updateCRC(t.payload);
    	if(sendPackage3Times(t, &y) == -1)
    		return -1;
    	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

    }
   
    /* Cat timp mai exista fisiere, se realizeaza succesiune de trimitere de pachete File-Header, serie 
    de pachete de date si EOF */

    for(int i = 1; i < argc; i++){
		//Trimitere File-Header
		F = init_F(argv[i]);
		F.SEQ = (y->payload[2] + 1)%64;
		package2payloadData(F, t.payload);
		t.len = (int) (t.payload[1] + 2);
		updateCRC(t.payload);

		if(sendPackage3Times(t, &y) == -1)
	    	return -1;
		printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

		while(y->payload[3] == 'N'){
	    	t.payload[2] = (y->payload[2] + 1) % 64;
	    	updateCRC(t.payload);
	    	if(sendPackage3Times(t, &y) == -1)
	    		return -1;
	    	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);
	    }


	    //Deschidere fisier de input si citirea in grupuri de 250 de octeti a datelor binare

		fp = fopen(argv[i], "rb");
		size_t bytes_read = 0;
		
		int length;
		int count = 0;
		/* Cat timp se mai poate citi din fisier, se creeaza pachete de date in functie de numarul de octeti
		care s-au citit din fisier (bytes_read), iar apoi se trimite fiecare pachet de date in parte */
		while( (bytes_read = fread(read_buffer, sizeof(unsigned char), 250, fp)) != 0){
			D = init_D(read_buffer, bytes_read);
			D.SEQ = (y->payload[2] + 1) % 64;
			package2payloadDataBytes(D, t.payload, bytes_read);
			updateCRCFiles(t.payload);
			length = (unsigned char) t.payload[1];
			t.len = length + 2;
			if(sendPackage3Times(t, &y) == -1)
		    	return -1;
			printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

			while(y->payload[3] == 'N'){
		    	t.payload[2] = (y->payload[2] + 1) % 64;
		    	updateCRCFiles(t.payload);
		    	if(sendPackage3Times(t, &y) == -1)
		    		return -1;
		    	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);
		    }
		    count++;
		}
		fclose(fp);

		//Trimitere pachet de tip EOF

		Z = init_Z();
		Z.SEQ = (y->payload[2] + 1) % 64;
		package2payloadNullData(Z, t.payload);
		t.len = (int) (t.payload[1] + 2);
		updateCRC(t.payload);
	    
	    if(sendPackage3Times(t, &y) == -1)
	    	return -1;
		printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

		while(y->payload[3] == 'N'){
	    	t.payload[2] = (y->payload[2] + 1) % 64;
	    	updateCRC(t.payload);
	    	if(sendPackage3Times(t, &y) == -1)
	    		return -1;
	    	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);
	    }
	}

    //Trimitere pachet de tip EOT

	B = init_B();
	B.SEQ = (y->payload[2] + 1) % 64;
	package2payloadNullData(B, t.payload);
	t.len = (int) (t.payload[1] + 2);
	updateCRC(t.payload);
    
    if(sendPackage3Times(t, &y) == -1)
    	return -1;
	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);

	while(y->payload[3] == 'N'){
    	t.payload[2] = (y->payload[2] + 1) % 64; 
    	updateCRC(t.payload);
    	if(sendPackage3Times(t, &y) == -1)
    		return -1;
    	printf("[%s] [%d]Got reply of type [%c]\n", argv[0], y->payload[2], y->payload[3]);
    }

    if(y->payload[3] == 'Y')
    	return -1;
    return 0;
}
