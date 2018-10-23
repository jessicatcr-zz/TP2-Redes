#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include "tp_socket.h"

int main(int argc, char const *argv[]) {
    // Tempo inicial
    struct timeval t0, t1;
    gettimeofday(&t0, 0);

    // Argumentos
    char *host_do_servidor = argv[1];
    char *porto_servidor = argv[2];
    char *nome_arquivo = argv[3];
    char *tam_buffer = argv[4];

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
    char* ultimoDatagramaRecv = (char*) malloc(atoi(tam_buffer) * sizeof(char));

    // Função de inicialização
    printf("Iniciando Client\n");
    tp_init();

    // Criando o socket para o cliente, sem bind.
    server_fd = tp_socket(porta, false);
    if (server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    else
        printf("Servidor socket criado com fd %d\n", server_fd);

    // Envia o nome do arquivo
    tp_sendto(server_fd, nome_arquivo, atoi(tam_buffer), &local_addr);

    // Recebe a confirmação do servidor com o tamanho do arquivo
    tp_recvfrom(server_fd, datagramaRecv, atoi(tam_buffer), &local_addr);

    // Depois de criar o socket e receber as informações do arquivo, coloca um timeout de 1 segundo.
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    if(datagramaRecv[0] == '3')
    {
        printf("Erro na abertura do arquivo no servidor\n");

        close(server_fd);
    }
    else if(datagramaRecv[0] == '2')
    {
        int filesize;
        int recebidos = 0;
        FILE *fp = fopen(nome_arquivo, "w");

        if (fp == NULL)
        {
            printf("Erro na cria do arquivo\n");
            close(server_fd);
        }
        else
        {
            printf("Arquivo aberto com sucesso no servidor\n");

            do
            {
                datagramaSend[0] = '1';
                sprintf(datagramaSend + 1, "%i", recebidos);
                tp_sendto(server_fd, datagramaSend, atoi(tam_buffer), &local_addr);

                n = tp_recvfrom(server_fd, datagramaRecv, atoi(tam_buffer), &local_addr);

                if(n == -1 && errno == 11){
					printf("Timeout. Enviando confirmação novamente. \n");
					continue; // Envia uma confirmação novamente em caso de timeout
				}
                    

                if(strcmp(ultimoDatagramaRecv, datagramaRecv) == 0){
					printf("Datagrama repetido. Enviando confirmação novamente. \n");
    				continue; // Envia uma confirmação novamente em caso de datagrama repetido
				}                
                else
                    strcpy(ultimoDatagramaRecv, datagramaRecv);

                fprintf(fp, "%s", datagramaRecv + 1); // Salva os dados recebidos no arquivo
                recebidos += (n - 1);

                if(datagramaRecv[0] == '0')
                    break; // Para caso o servidor tenha chegado no final do arquivo
                else if(datagramaRecv[0] == '1')
                {
                    // Limpando os buffers
                    memset(datagramaSend, 0x0, atoi(tam_buffer));
                    memset(datagramaRecv, 0x0, atoi(tam_buffer));
                    continue;
                }

            }while(true);

            datagramaSend[0] = '0';
            sprintf(datagramaSend, "%i", recebidos);
            tp_sendto(server_fd, datagramaSend, atoi(tam_buffer), &local_addr); // Envia confirmação de recebimento do final do arquivo

            fclose(fp);
            fprintf(stdout, "\n\nConnection closed\n\n");
            close(server_fd);
        }
    }

    // Tempo final
    gettimeofday(&t1, 0);
    int tempo = (t1.tv_sec - t0.tv_sec);
    printf("Tempo gasto: %i\n", tempo);

    return 0;
}
