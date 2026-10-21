/*
 * Scoundrel - jogo de cartas solitario em C (padrao C99)
 * Compilar:  gcc -std=c99 -Wall -Wextra -pedantic -o scoundrel scoundrel.c
 * Executar:  ./scoundrel   (Windows: scoundrel.exe)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define VIDA_MAXIMA    20
#define TOTAL_CARTAS   44
#define TAMANHO_SALA   4
#define SEM_LIMITE     99   /* arma nova: pode atacar qualquer monstro */

typedef enum { MONSTRO, ARMA, POCAO } TipoCarta;
typedef enum { PAUS, ESPADAS, COPAS, OUROS } Naipe;

typedef struct {
    Naipe naipe;
    TipoCarta tipo;
    int valor;
} Carta;

typedef struct {
    int vida;
    int arma_equipada;
    int ultimo_monstro_morto;
    int pocao_usada_no_turno;
    Carta ultima_carta_jogada;
    int jogou_carta;
} Jogador;

static const char *obter_nome_naipe(Naipe naipe) {
    switch (naipe) {
        case PAUS:    return "Paus (M)";
        case ESPADAS: return "Espadas (M)";
        case COPAS:   return "Copas (P)";
        case OUROS:   return "Ouros (A)";
        default:      return "?";
    }
}

static void imprimir_carta(Carta c) {
    printf("[ ");
    if (c.valor <= 10)       printf("%d", c.valor);
    else if (c.valor == 11)  printf("J");
    else if (c.valor == 12)  printf("Q");
    else if (c.valor == 13)  printf("K");
    else                     printf("A");
    printf(" de %s ]", obter_nome_naipe(c.naipe));
}

/* Le uma linha e devolve o primeiro caractere em minusculo.
 * Devolve '\0' em caso de EOF (Ctrl+D / Ctrl+Z / entrada encerrada). */
static char ler_entrada(const char *mensagem) {
    char buffer[64];
    printf("%s", mensagem);
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return '\0';
    }

    /* Se a linha foi maior que o buffer, descarta o resto para nao
     * contaminar a proxima leitura. */
    if (strchr(buffer, '\n') == NULL) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }

    return (char)tolower((unsigned char)buffer[0]);
}

static int criar_baralho(Carta masmorra[]) {
    int total = 0;

    /* Monstros: Paus e Espadas, 2..14 (A = 14) -> 26 cartas */
    for (int naipe = PAUS; naipe <= ESPADAS; naipe++) {
        for (int valor = 2; valor <= 14; valor++) {
            masmorra[total].naipe = (Naipe)naipe;
            masmorra[total].tipo  = MONSTRO;
            masmorra[total].valor = valor;
            total++;
        }
    }
    /* Pocoes: Copas 2..10 -> 9 cartas */
    for (int valor = 2; valor <= 10; valor++) {
        masmorra[total].naipe = COPAS;
        masmorra[total].tipo  = POCAO;
        masmorra[total].valor = valor;
        total++;
    }
    /* Armas: Ouros 2..10 -> 9 cartas */
    for (int valor = 2; valor <= 10; valor++) {
        masmorra[total].naipe = OUROS;
        masmorra[total].tipo  = ARMA;
        masmorra[total].valor = valor;
        total++;
    }
    return total;
}

static void embaralhar_baralho(Carta masmorra[], int total) {
    for (int i = total - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta temp = masmorra[i];
        masmorra[i] = masmorra[j];
        masmorra[j] = temp;
    }
}

static void executar_partida(void) {
    Carta masmorra[TOTAL_CARTAS];
    Carta sala[TAMANHO_SALA];
    Carta escolhida;
    Jogador jogador;

    int qtd_masmorra;
    int qtd_sala = 0;          /* quantidade de espacos ocupados na sala */
    int ocupada[TAMANHO_SALA] = {0};  /* 1 = espaco com carta, 0 = vazio */
    int qtd_descarte = 0;      /* so contagem: o descarte nao e reutilizado */
    int cartas_enfrentadas = 0;
    int pode_pular = 1;
    int indice, pontuacao_final;
    char escolha;

    memset(&jogador, 0, sizeof(jogador));

    qtd_masmorra = criar_baralho(masmorra);
    embaralhar_baralho(masmorra, qtd_masmorra);

    jogador.vida = VIDA_MAXIMA;
    jogador.arma_equipada = 0;
    jogador.ultimo_monstro_morto = SEM_LIMITE;
    jogador.pocao_usada_no_turno = 0;

    printf("\n=========================================\n");
    printf("   INICIANDO NOVA PARTIDA DE SCOUNDREL   \n");
    printf("=========================================\n");
    printf("(Pressione 'x' a qualquer momento para voltar ao Menu)\n\n");

    while (jogador.vida > 0) {
        /* A sala so e reabastecida no inicio de um turno (partida nova,
         * depois de 3 cartas enfrentadas ou depois de pular a sala).
         * Apenas os espacos vazios recebem cartas novas; a carta que
         * sobrou do turno anterior permanece onde estava.
         * (topo do baralho = ultimo indice) */
        if (cartas_enfrentadas == 0) {
            for (int i = 0; i < TAMANHO_SALA && qtd_masmorra > 0; i++) {
                if (!ocupada[i]) {
                    sala[i] = masmorra[--qtd_masmorra];
                    ocupada[i] = 1;
                    qtd_sala++;
                }
            }
        }

        /* Vitoria */
        if (qtd_sala == 0 && qtd_masmorra == 0) {
            printf("\n*** PARABENS! VOCE COMPLETOU A MASMORRA! ***\n");
            pontuacao_final = jogador.vida;
            if (jogador.vida == VIDA_MAXIMA && jogador.jogou_carta &&
                jogador.ultima_carta_jogada.tipo == POCAO) {
                pontuacao_final += jogador.ultima_carta_jogada.valor;
            }
            printf("Pontuacao final: %d\n\n", pontuacao_final);
            return;
        }

        /* Maior monstro atacavel com a arma: estritamente menor que o ultimo morto */
        printf("\n-----------------------------------------\n");
        printf("VIDA: %d/%d | ARMA: %d (Max Monstro: %d)\n",
               jogador.vida, VIDA_MAXIMA, jogador.arma_equipada,
               (jogador.ultimo_monstro_morto == SEM_LIMITE
                    ? 14 : jogador.ultimo_monstro_morto - 1));
        printf("Cartas na Masmorra: %d | Descarte: %d\n", qtd_masmorra, qtd_descarte);
        printf("-----------------------------------------\n");
        printf("SALA ATUAL:\n");
        for (int i = 0; i < TAMANHO_SALA; i++) {
            printf(" [%d] ", i + 1);
            if (ocupada[i]) {
                imprimir_carta(sala[i]);
            } else {
                printf("(vazio)");
            }
            printf("\n");
        }

        printf("\nOpcoes:\n");
        printf(" (1-%d) Enfrentar carta (espacos marcados como vazio nao tem carta)\n",
               TAMANHO_SALA);
        if (pode_pular && cartas_enfrentadas == 0 && qtd_sala == TAMANHO_SALA) {
            printf(" (f) Pular sala (Skip Room)\n");
        }
        printf(" (x) Abandonar e voltar ao Menu\n");

        escolha = ler_entrada("\nSua escolha: ");

        if (escolha == 'x' || escolha == '\0') {
            printf("\nPartida abandonada.\n\n");
            return;
        }

        /* Pular sala: as 4 cartas vao para o fundo (indice 0) do baralho */
        if (escolha == 'f' && pode_pular && cartas_enfrentadas == 0 &&
            qtd_sala == TAMANHO_SALA) {
            printf("\nVoce pulou a sala! As cartas voltam para o fundo da Masmorra.\n");
            memmove(&masmorra[qtd_sala], &masmorra[0],
                    (size_t)qtd_masmorra * sizeof(Carta));
            memcpy(&masmorra[0], sala, (size_t)qtd_sala * sizeof(Carta));
            qtd_masmorra += qtd_sala;
            for (int i = 0; i < TAMANHO_SALA; i++) ocupada[i] = 0;
            qtd_sala = 0;
            pode_pular = 0;
            continue;
        }

        /* 'f' chegou aqui: pular a sala nao e permitido neste momento */
        if (escolha == 'f') {
            if (!pode_pular) {
                printf("\nVoce nao pode pular duas salas seguidas!\n");
            } else {
                printf("\nSo e possivel pular a sala antes de enfrentar qualquer carta.\n");
            }
            continue;
        }

        indice = escolha - '1';
        if (indice < 0 || indice >= TAMANHO_SALA) {
            printf("\nOpcao invalida! Tente novamente.\n");
            continue;
        }
        if (!ocupada[indice]) {
            printf("\nO espaco %d esta vazio! Escolha outra carta.\n", indice + 1);
            continue;
        }

        escolhida = sala[indice];
        jogador.ultima_carta_jogada = escolhida;
        jogador.jogou_carta = 1;

        if (escolhida.tipo == POCAO) {
            if (!jogador.pocao_usada_no_turno) {
                int vida_antes = jogador.vida;
                jogador.vida += escolhida.valor;
                if (jogador.vida > VIDA_MAXIMA) jogador.vida = VIDA_MAXIMA;
                jogador.pocao_usada_no_turno = 1;
                printf("\nPocao usada! Curou %d. Vida atual: %d\n",
                       jogador.vida - vida_antes, jogador.vida);
            } else {
                printf("\nVoce ja usou uma pocao neste turno! Esta foi descartada e curou 0. Vida atual: %d\n",
                       jogador.vida);
            }
            qtd_descarte++;
        }
        else if (escolhida.tipo == ARMA) {
            jogador.arma_equipada = escolhida.valor;
            jogador.ultimo_monstro_morto = SEM_LIMITE;
            printf("\nNova arma equipada! Poder: %d\n", jogador.arma_equipada);
            qtd_descarte++;
        }
        else { /* MONSTRO */
            /* A arma so derrota monstros de valor ESTRITAMENTE MENOR que o
             * ultimo monstro que ela matou. */
            int pode_usar_arma = (jogador.arma_equipada > 0) &&
                             (escolhida.valor < jogador.ultimo_monstro_morto);
            int escolha_combate = 1;

            if (pode_usar_arma) {
                int dano_com_arma = escolhida.valor - jogador.arma_equipada;
                if (dano_com_arma < 0) dano_com_arma = 0;

                printf("\nMonstro selecionado (%d de dano).\n", escolhida.valor);
                printf("1. Lutar com Maos Limpas (Recebe %d de dano)\n", escolhida.valor);
                printf("2. Usar Arma Equipada (Recebe %d de dano)\n", dano_com_arma);

                char c = ler_entrada("Escolha o combate (1-2): ");
                if (c == 'x' || c == '\0') {
                    printf("\nPartida abandonada.\n\n");
                    return;
                }
                if (c == '2') escolha_combate = 2;
            }

            if (escolha_combate == 2) {
                int dano = escolhida.valor - jogador.arma_equipada;
                if (dano < 0) dano = 0;
                jogador.vida -= dano;
                jogador.ultimo_monstro_morto = escolhida.valor;
                printf("\nUsou a arma e recebeu %d de dano!\n", dano);
            } else {
                jogador.vida -= escolhida.valor;
                printf("\nLuta com maos limpas! Recebeu %d de dano!\n", escolhida.valor);
            }
            qtd_descarte++;
        }

        /* Esvazia o espaco da carta escolhida (as demais nao se movem) */
        ocupada[indice] = 0;
        qtd_sala--;
        cartas_enfrentadas++;

        if (cartas_enfrentadas == 3) {
            cartas_enfrentadas = 0;
            jogador.pocao_usada_no_turno = 0;
            pode_pular = 1;
            printf("\nTurno concluido! A carta restante segue para a proxima sala.\n");
        }
    }

    /* Derrota */
    printf("\n=========================================\n");
    printf("               VOCE MORREU!              \n");
    printf("=========================================\n");

    int soma_monstros = 0;
    for (int i = 0; i < qtd_masmorra; i++) {
        if (masmorra[i].tipo == MONSTRO) soma_monstros += masmorra[i].valor;
    }
    for (int i = 0; i < TAMANHO_SALA; i++) {
        if (ocupada[i] && sala[i].tipo == MONSTRO) soma_monstros += sala[i].valor;
    }

    pontuacao_final = jogador.vida - soma_monstros;
    printf("Pontuacao Final: %d\n\n", pontuacao_final);
}

int main(void) {
    char opcao;
    srand((unsigned int)time(NULL));

    for (;;) {
        printf("=========================================\n");
        printf("           SCOUNDREL - C RPG             \n");
        printf("=========================================\n");
        printf("1. Jogar\n");
        printf("2. Sair\n");

        opcao = ler_entrada("Escolha uma opcao: ");

        if (opcao == '1') {
            executar_partida();
        } else if (opcao == '2' || opcao == '\0') {
            printf("\nSaindo do jogo... Ate logo!\n");
            break;
        } else {
            printf("\nOpcao invalida. Digite 1 ou 2.\n\n");
        }
    }

    return 0;
}