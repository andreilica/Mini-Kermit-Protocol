#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

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


/* Functie folosita pentru a muta toate datele din pachetul Send-Init in Payload */

void package2payloadInit(package pack, char *payload){
    char *copy = payload;
    memcpy(copy, &pack.SOH, sizeof(pack.SOH));
    memcpy(copy += sizeof(pack.SOH), &pack.LEN, sizeof(pack.LEN));
    memcpy(copy += sizeof(pack.LEN), &pack.SEQ, sizeof(pack.SEQ));
    memcpy(copy += sizeof(pack.SEQ), &pack.TYPE, sizeof(pack.TYPE));
    memcpy(copy += sizeof(pack.TYPE), &pack.DATA, 11);
    memcpy(copy += 11, &pack.CHECK, sizeof(pack.CHECK));
    memcpy(copy += sizeof(pack.CHECK), &pack.MARK, sizeof(pack.MARK));
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

/* Functie folosita pentru a initializa si crea un pachet de tip 'N' */

package init_N(){
    package N_Package;
    char EOL = 0x0D;
    char aux_string_crc[50];

    N_Package.SOH = 0x01;
    N_Package.LEN = 0x05;
    N_Package.SEQ = 0x00;
    N_Package.TYPE = 'N';

    aux_string_crc[0] = N_Package.SOH;
    aux_string_crc[1] = N_Package.LEN;
    aux_string_crc[2] = N_Package.SEQ;
    aux_string_crc[3] = N_Package.TYPE;

    N_Package.CHECK = crc16_ccitt(aux_string_crc, 4);
    N_Package.MARK = EOL;

    return N_Package;
}

/* Functie folosita pentru a initializa si crea un pachet de tip 'Y' */

package init_Y(){
    package Y_Package;
    char EOL = 0x0D;
    char aux_string_crc[50];

    Y_Package.SOH = 0x01;
    Y_Package.LEN = 0x05;
    Y_Package.SEQ = 0x00;
    Y_Package.TYPE = 'Y';

    aux_string_crc[0] = Y_Package.SOH;
    aux_string_crc[1] = Y_Package.LEN;
    aux_string_crc[2] = Y_Package.SEQ;
    aux_string_crc[3] = Y_Package.TYPE;

    Y_Package.CHECK = crc16_ccitt(aux_string_crc, 4);
    Y_Package.MARK = EOL;

    return Y_Package;
}

/* Functie folosita pentru a initializa si creea un pachet de tip 'Y' corespunzator pachetului
Send-Init
 */

package init_YS(){
    package YS_Package;
    char MAXL = 250, TIME = 5, NPAD = 0x00, PADC = 0x00, EOL = 0x0D;
    char QCTL = 0x00, QBIN = 0x00, CHKT = 0x00, REPT = 0x00, CAPA = 0x00, R = 0x00;
    char aux_string_crc[50];

    YS_Package.SOH = 0x01;
    YS_Package.LEN = 0x10;
    YS_Package.SEQ = 0x00;
    YS_Package.TYPE = 'Y';
    YS_Package.DATA[0] = MAXL;
    YS_Package.DATA[1] = TIME;
    YS_Package.DATA[2] = NPAD;
    YS_Package.DATA[3] = PADC;
    YS_Package.DATA[4] = EOL;
    YS_Package.DATA[5] = QCTL;
    YS_Package.DATA[6] = QBIN;
    YS_Package.DATA[7] = CHKT;
    YS_Package.DATA[8] = REPT;
    YS_Package.DATA[9] = CAPA;
    YS_Package.DATA[10] = R;
    YS_Package.MARK = EOL;

    aux_string_crc[0] = YS_Package.SOH;
    aux_string_crc[1] = YS_Package.LEN;
    aux_string_crc[2] = YS_Package.SEQ;
    aux_string_crc[3] = YS_Package.TYPE;

    char *p = aux_string_crc;
    memcpy(p += 4, &YS_Package.DATA, 11);
    YS_Package.CHECK = crc16_ccitt(aux_string_crc, 15);
    return YS_Package;
}

/* Functie folosita pentru a verifica daca pachetul primit are acelasi CRC cu cel din campul CHECK corespunzator */ 

int checkCRC(char *received_payload, unsigned short *crc){
    char aux_string[260];
    char aux_crc[2];

    memcpy(aux_string, received_payload, received_payload[1] - 1);
    (*crc) = crc16_ccitt(aux_string, received_payload[1] - 1);
    aux_crc[0] = (*crc) & 0xff;
    aux_crc[1] = ((*crc) >> 8) & 0xff;

    if(aux_crc[0] == received_payload[received_payload[1] - 1] && aux_crc[1] == received_payload[(int)received_payload[1]])
        return 1;
    return 0;
}

/* Functie folosita pentru a verifica daca pachetul primit are acelasi CRC cu cel din campul CHECK corespunzator in cazul in care
campul LEN al pachetului depaseste 127 si este interpretat ca fiind signed char, fiind necesar un cast la unsigned char */ 

int checkCRCFiles(char *received_payload, unsigned short *crc){
    char aux_string[260];
    char aux_crc[2];
    int length = (unsigned char) received_payload[1];
    memcpy(aux_string, received_payload, length - 1);
    (*crc) = crc16_ccitt(aux_string, length - 1);
    aux_crc[0] = (*crc) & 0xff;
    aux_crc[1] = ((*crc) >> 8) & 0xff;
    if(aux_crc[0] == received_payload[length - 1] && aux_crc[1] == received_payload[length])
        return 1;
    return 0;
}



int main(int argc, char** argv) {
    msg t;
    msg *r;
    FILE *fp;
    unsigned short crc;
    init(HOST, PORT);

    char result_file[30] = "recv_";
    char data_buffer[250];

    char *pointer1;
    char *pointer2;

    //Initializarea a doua pachete de tip NACK, respectiv ACK

    package NACK_INIT = init_N();
    package ACK_INIT_NULL = init_Y();


    /* Se asteapta primirea pachetului de tip Send-Init de inca 2 ori TIME, iar in cazul in care nu s-a primit
    se va termina conexiunea */
    r = receive_message_timeout(5000);
    int wait_count = 0;
    while(r == NULL){
        r = receive_message_timeout(5000);
        wait_count++;
        if(wait_count == 3){
            printf("Inchidere transmisiune!\n");
            return -1;
        }
    }

    printf("[%s] [%d]Got Send-Init package\n", argv[0], r->payload[2]);

    /* Se verifica CRC-ul pachetului primit, iar in cazul in care acesta nu este corect, se va trimite un pachet de tip NACK
    si se va astepta primirea pachetului corect. In cazul in care aceasta operatiune esueaza de 3 ori, transmisiunea se va inchide */

    while(checkCRC(r->payload, &crc) == 0){
        package2payloadNullData(NACK_INIT, t.payload);
        t.len = (int) (t.payload[1] + 2);
        t.payload[2] = r->payload[2];
        send_message(&t);

        r = receive_message_timeout(5000);
        wait_count = 0;
        while(r == NULL){
            r = receive_message_timeout(5000);
            wait_count++;
            printf("Se asteapta primirea mesajului, count: %d", wait_count);
            if(wait_count == 3){
                printf("Inchidere transmisiune!\n");
                return -1;
            }
        }


        printf("[%s] [%d]Got Send-Init package\n", argv[0], r->payload[2]);
    }

    /* La iesirea din aceasta bucla, in caz ca nu s-a terminat conexiunea, pachetul primit va fi cel corect
    si se va trimite un packet de tip ACK pentru acesta, cu acelasi numar de secventa cu pachetul pentru
    care raspunde */

    package ACK_INIT = init_YS();
    package2payloadInit(ACK_INIT, t.payload);
    t.payload[2] = r->payload[2];
    t.len = (int) (t.payload[1] + 2);
    send_message(&t);


    /* Se asteapta primirea primului pachet de tip File-Header, iar in cazul in care nu s-a primit
    se va termina conexiunea */
    r = receive_message_timeout(5000);
    wait_count = 0;
        while(r == NULL){
            r = receive_message_timeout(5000);
            wait_count++;
            if(wait_count == 3){
                printf("Inchidere transmisiune!\n");
                return -1;
            }
        }


    /* Cat timp pachetul curent este diferit de EOT, se va repeta acest ciclu in are se primeste un file-header,
    o serie de pachete de date si un pachet de tip EOF */
    while(r->payload[3] != 'B'){
        //Primire packet File-Header
        printf("[%s] [%d]Got File-Header package\n", argv[0], r->payload[2]);
        
        while(checkCRC(r->payload, &crc) == 0){
            package2payloadNullData(NACK_INIT, t.payload);
            t.len = (int) (t.payload[1] + 2);
            t.payload[2] = r->payload[2];
            send_message(&t);

            r = receive_message_timeout(5000);
            wait_count = 0;
            while(r == NULL){
                r = receive_message_timeout(5000);
                wait_count++;
                if(wait_count == 3){
                    printf("Inchidere transmisiune!\n");
                    return -1;
                }
            }

            printf("[%s] [%d]Got File-Header package\n", argv[0], r->payload[2]);
        }

        /* Se creeaza fisierul de output corespunzator file-header-ului primit, adaugandu-se recv_ in fata numelui fisierului */

        pointer1 = result_file;
        pointer2 = r->payload;
        pointer1 += 5;
        pointer2 += 4;
        memcpy(pointer1, pointer2, r->len - 7);
        fp = fopen(result_file, "wb");

        ACK_INIT_NULL = init_Y();
        package2payloadNullData(ACK_INIT_NULL, t.payload);
        t.payload[2] = r->payload[2];
        t.len = (int) (t.payload[1] + 2);
        send_message(&t);
        

        //Primire serie de pachete de date
        r = receive_message_timeout(5000);
        wait_count = 0;
        while(r == NULL){
            r = receive_message_timeout(5000);
            wait_count++;
            if(wait_count == 3){
                printf("Inchidere transmisiune!\n");
                return -1;
            }
        }

        /* Cat timp pachetul curent este diferit de EOF, se vor astepta pachete de date ce vor fi prelucrate,
        iar datele din ele vor fi copiate in fisierul de output */
        while(r->payload[3] != 'Z'){
            printf("[%s] [%d]Got data package\n", argv[0], r->payload[2]);

            while(checkCRCFiles(r->payload, &crc) == 0){
                package2payloadNullData(NACK_INIT, t.payload);
                t.len = (int) (t.payload[1] + 2);
                t.payload[2] = r->payload[2];
                send_message(&t);

                r = receive_message_timeout(5000);
                wait_count = 0;
                while(r == NULL){
                    r = receive_message_timeout(5000);
                    wait_count++;
                    if(wait_count == 3){
                        printf("Inchidere transmisiune!\n");
                        return -1;
                    }
                }

                printf("[%s] [%d]Got data package\n", argv[0], r->payload[2]);
            }


            //Prelucrare pachet de date + scriere in fisierul de output */
            pointer1 = data_buffer;
            pointer2 = r->payload;
            pointer2 += 4;
            memcpy(pointer1, pointer2, r->len - 7);
            fwrite(data_buffer, sizeof(unsigned char), r->len - 7, fp);

            ACK_INIT_NULL = init_Y();
            package2payloadNullData(ACK_INIT_NULL, t.payload);
            t.payload[2] = r->payload[2];
            t.len = (int) (t.payload[1] + 2);
            send_message(&t);

            r = receive_message_timeout(5000);
            wait_count = 0;
            while(r == NULL){
                r = receive_message_timeout(5000);
                wait_count++;
                if(wait_count == 3){
                    printf("Inchidere transmisiune!\n");
                    return -1;
                }
            }
    	}

        fclose(fp);

        //Primire pachet de tip EOF

        printf("[%s] [%d]Got EOF package\n", argv[0], r->payload[2]);
        while(checkCRC(r->payload, &crc) == 0){
            package2payloadNullData(NACK_INIT, t.payload);
            t.len = (int) (t.payload[1] + 2);
            t.payload[2] = r->payload[2];
            send_message(&t);

            r = receive_message_timeout(5000);
            wait_count = 0;
            while(r == NULL){
                r = receive_message_timeout(5000);
                wait_count++;
                if(wait_count == 3){
                    printf("Inchidere transmisiune!\n");
                    return -1;
                }
            }

            printf("[%s] [%d]Got EOF package\n", argv[0], r->payload[2]);
        }

        ACK_INIT_NULL = init_Y();
        package2payloadNullData(ACK_INIT_NULL, t.payload);
        t.payload[2] = r->payload[2];
        t.len = (int) (t.payload[1] + 2);
        send_message(&t);


        /* Primirea urmatorului pachet dupa ultimul pachet de tip EOF.
        Daca acesta este diferit de EOT, se va relua tot ciclul parcurs anterior. */

        r = receive_message_timeout(5000);
        wait_count = 0;
        while(r == NULL){
            r = receive_message_timeout(5000);
            wait_count++;
            if(wait_count == 3){
                printf("Inchidere transmisiune!\n");
                return -1;
            }
        }

    }

    /* Verificarea corectitudinii pachetului EOT primit si trimiterea ACK-ului sau NACK-ului pentru acesta */
    
    printf("[%s] [%d]Got EOT package\n", argv[0], r->payload[2]);
    while(checkCRC(r->payload, &crc) == 0){
        package2payloadNullData(NACK_INIT, t.payload);
        t.len = (int) (t.payload[1] + 2);
        t.payload[2] = r->payload[2];
        send_message(&t);

        r = receive_message_timeout(5000);
        wait_count = 0;
        while(r == NULL){
            r = receive_message_timeout(5000);
            wait_count++;
            if(wait_count == 3){
                printf("Inchidere transmisiune!\n");
                return -1;
            }
        }

        printf("[%s] [%d]Got EOT package\n", argv[0], r->payload[2]);

    }

    ACK_INIT_NULL = init_Y();
    package2payloadNullData(ACK_INIT_NULL, t.payload);
    t.payload[2] = r->payload[2];
    t.len = (int) (t.payload[1] + 2);
    send_message(&t);


	return 0;
}
