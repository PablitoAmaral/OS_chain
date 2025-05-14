#include <stdio.h>
#include "statistics.h"
#include <signal.h>  

void handle_stats_sigusr1(int sig) {
      // POR IMPLEMENTAR  DAR PRINT A ESTATISTICAS---------------------------------------------------------------------
  }


void run_statistics(void) {
    signal(SIGUSR1, handle_stats_sigusr1); // SIGUSR1 signal handler

    printf("[Statistics] Processo statistics iniciado!\n");
    while (1); // manter ativo para testes
}