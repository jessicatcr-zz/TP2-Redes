#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include "tp_socket.h"

int main(int argc, char *argv[]) {

    // Argumentos
    char *porto_servidor = argv[1];
    char *tam_buffer = argv[2];

    // Sockets
    int server_fd, client_fd;
    int opt = 1;
    int addrlen = sizeof(local_addr);

    // Buffer
    char* datagramaSend = (char*) malloc(atoi(tam_buffer) * sizeof(char));
    char* datagramaRecv = (char*) malloc(atoi(tam_buffer) * sizeof(char));

    // Auxiliares
    int n;
    unsigned short porta = atoi(porto_servidor);
    char* nome_arquivo = (char*) malloc(atoi(tam_buffer) * sizeof(char));

    // Função de inicialização
    printf("Iniciando Servidor\n");
    tp_init();

    // Criando Ipv4 socket, definindo as propriedades do socket, usando UDP
    // e fazendo o bind do socket com a porta
    server_fd = tp_socket(porta, true);
    if (server_fd == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    else
        printf("Servidor socket criado com fd %d\n", server_fd);

    //Recebe o nome do arquivo
    n = tp_recvfrom(server_fd, nome_arquivo, atoi(tam_buffer), &local_addr);

    // Tempo inicial
    struct timeval t0, t1;
    gettimeofday(&t0, 0);

    if(n > 0)
    {
        nome_arquivo[n] = '\0';
        printf("Nome do arquivo %s\n", nome_arquivo);
    }

    // Antes de iniciar o envio do arquivo, coloca um timeout de 1 segundo.
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    memset(datagramaSend, 0x0, atoi(tam_buffer));
    memset(datagramaRecv, 0x0, atoi(tam_buffer));

    // Tenta abrir o arquivo
    FILE *fp = fopen(nome_arquivo, "r");
    if (fp == NULL)
    {
        printf("Erro na abertura do arquivo\n");

        // Envia cabeçalho 3 e datagrama vazio
        datagramaSend[0] = '3';
        tp_sendto(server_fd, datagramaSend, atoi(tam_buffer), &local_addr);

        close(server_fd);
    }
    else
    {
        int filesize;

        printf("Arquivo aberto com sucesso\n");

        // Calcula o tamanho do arquivo
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // Envia cabeçalho 2 e o tamanho do arquivo
        datagramaSend[0] = '2';
        sprintf(datagramaSend + 1, "%i", filesize);
        tp_sendto(server_fd, datagramaSend, atoi(tam_buffer), &local_addr);

        // Recebe confirmação antes de iniciar o envio do arquivo
        tp_recvfrom(server_fd, datagramaRecv, atoi(tam_buffer), &local_addr);

        int enviados = 0, recebidos = 0;

        while(!feof(fp))
        {
            fread(datagramaSend + 1, 1, (atoi(tam_buffer) - 1), fp);

            // Envia cabeçalho 0 caso a leitura do arquivo já tenha sido realizada, e 1 caso ainda existam dados a serem enviados.
            if(feof(fp))
                datagramaSend[0] = '0';
            else
                datagramaSend[0] = '1';

            do
            {
                tp_sendto(server_fd, datagramaSend, atoi(tam_buffer), &local_addr);
                n = tp_recvfrom(server_fd, datagramaRecv, atoi(tam_buffer), &local_addr);

                if(n == -1 && errno == 11){
					printf("Timeout. Enviando confirmação novamente. \n");
    				continue; // envia os dados novamente em caso de timeout
				}                

                sscanf(datagramaRecv + 1, "%i", &recebidos);

                if(recebidos != enviados){
                    enviados = recebidos;
                    break;
                }
                else if(recebidos == enviados && datagramaRecv[0] == '0')
                    break;

                //if(feof(fp) || datagramaRecv[0] == '0')
                //    break;

            } while(true);

            // Limpando os buffers
            memset(datagramaSend, 0x0, atoi(tam_buffer));
            memset(datagramaRecv, 0x0, atoi(tam_buffer));
        }

        fclose(fp);
        fprintf(stdout, "\n\nConnection closed\n\n");
        close(server_fd);
    }

    // Tempo final
    gettimeofday(&t1, 0);
    int tempo = (t1.tv_sec - t0.tv_sec);
    printf("Tempo gasto: %i\n", tempo);

    return 0;

}
