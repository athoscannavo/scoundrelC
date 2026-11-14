#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Configuracoes gerais do jogo */
#define VIDA_MAXIMA          20
#define TOTAL_CARTAS         52
#define TAMANHO_SALA         4
#define SEM_LIMITE_MONSTRO   99

/* Tipos de carta */
#define TIPO_MONSTRO         0
#define TIPO_ARMA            1
#define TIPO_POCAO           2

/* Naipes do baralho */
#define NAIPE_PAUS           0
#define NAIPE_ESPADAS        1
#define NAIPE_COPAS          2
#define NAIPE_OUROS          3

struct Carta {
    int naipe;
    int tipo;
    int valor;
};

struct Jogador {
    int vida;
    int arma_equipada;
    int ultimo_monstro_derrotado;
    int usou_pocao_no_turno;
    struct Carta ultima_carta_jogada;
};

/* Prototipos das funcoes */
static void exibir_instrucoes(void);
static const char *obter_nome_naipe(int naipe);
static void imprimir_carta(struct Carta carta);
static char ler_opcao_teclado(const char *mensagem);
static int criar_baralho(struct Carta *masmorra);
static void embaralhar_baralho(struct Carta *masmorra, int total_cartas);
static void exibir_estado_e_sala(struct Jogador jogador, struct Carta *sala,
                                 int *espaco_ocupado, int cartas_na_masmorra,
                                 int cartas_descartadas);
static void enfrentar_monstro(struct Jogador *jogador, struct Carta monstro,
                              int *abandonou_partida);
static int calcular_pontuacao_derrota(struct Jogador jogador,
                                      struct Carta *masmorra,
                                      int cartas_na_masmorra,
                                      struct Carta *sala,
                                      int *espaco_ocupado);
static void jogar_partida(void);

int main(void) {
    char opcao_menu;

    srand((unsigned int)time(NULL));
    exibir_instrucoes();

    while (1) {
        printf("=========================================================\n");
        printf("                 SCOUNDREL - JOGO DE CARTAS              \n");
        printf("=========================================================\n");
        printf("1. Jogar\n");
        printf("2. Ver Instrucoes\n");
        printf("3. Sair\n");

        opcao_menu = ler_opcao_teclado("Escolha uma opcao: ");

        if (opcao_menu == '1') {
            jogar_partida();
        } else if (opcao_menu == '2') {
            exibir_instrucoes();
        } else if (opcao_menu == '3' || opcao_menu == '\0') {
            printf("\nSaindo do jogo... Ate logo!\n");
            break;
        } else {
            printf("\nOpcao invalida. Digite 1, 2 ou 3.\n\n");
        }
    }

    return 0;
}

static void exibir_instrucoes(void) {
    printf("\n=========================================================\n");
    printf("               SOBRE O JOGO - SCOUNDREL                  \n");
    printf("=========================================================\n");
    printf("OBJETIVO:\n");
    printf("  Sobreviver e limpar todas as %d cartas da masmorra sem\n", TOTAL_CARTAS);
    printf("  deixar sua vida chegar a zero (vida maxima: %d).\n\n", VIDA_MAXIMA);
    printf("AS CARTAS:\n");
    printf("  - Paus e Espadas (2 a As): MONSTROS. Causam dano igual\n");
    printf("    ao seu valor (Valete=11, Dama=12, Rei=13, As=14).\n");
    printf("  - Ouros (2 a As): ARMAS. Reduzem o dano dos monstros.\n");
    printf("    Regra da arma: apos derrotar um monstro, ela so pode\n");
    printf("    atacar monstros mais fracos que o ultimo derrotado.\n");
    printf("  - Copas (2 a As): POCOES. Recuperam pontos de vida.\n");
    printf("    Apenas 1 pocao faz efeito por turno; outras sao\n");
    printf("    descartadas sem curar.\n\n");
    printf("COMO JOGAR:\n");
    printf("  - Cada sala mostra 4 cartas. Enfrente 3 delas; a 4a\n");
    printf("    carta permanece para formar a proxima sala.\n");
    printf("  - Voce pode fugir/pular uma sala inteira antes de\n");
    printf("    escolher cartas, mas nunca duas vezes seguidas.\n");
    printf("=========================================================\n\n");
}

static const char *obter_nome_naipe(int naipe) {
    switch (naipe) {
        case NAIPE_PAUS:    return "Paus (Monstro)";
        case NAIPE_ESPADAS: return "Espadas (Monstro)";
        case NAIPE_COPAS:   return "Copas (Pocao)";
        case NAIPE_OUROS:   return "Ouros (Arma)";
        default:            return "?";
    }
}

static void imprimir_carta(struct Carta carta) {
    printf("[ ");
    if (carta.valor <= 10)       printf("%d", carta.valor);
    else if (carta.valor == 11)  printf("J");
    else if (carta.valor == 12)  printf("Q");
    else if (carta.valor == 13)  printf("K");
    else                         printf("A");
    printf(" de %s ]", obter_nome_naipe(carta.naipe));
}

static char ler_opcao_teclado(const char *mensagem) {
    char texto_digitado[64];
    int caractere_extra;

    printf("%s", mensagem);
    fflush(stdout);

    if (fgets(texto_digitado, sizeof(texto_digitado), stdin) == NULL) {
        return '\0';
    }

    /* Limpa caracteres sobrando caso o usuario digite algo muito longo */
    if (strchr(texto_digitado, '\n') == NULL) {
        while ((caractere_extra = getchar()) != '\n' && caractere_extra != EOF) { }
    }

    return (char)tolower((unsigned char)texto_digitado[0]);
}

static int criar_baralho(struct Carta *masmorra) {
    int total_criado;
    int naipe_atual;
    int valor_atual;

    total_criado = 0;

    /* Monstros: Paus e Espadas de 2 ate 14 (As) -> 26 cartas */
    for (naipe_atual = NAIPE_PAUS; naipe_atual <= NAIPE_ESPADAS; naipe_atual++) {
        for (valor_atual = 2; valor_atual <= 14; valor_atual++) {
            masmorra[total_criado].naipe = naipe_atual;
            masmorra[total_criado].tipo  = TIPO_MONSTRO;
            masmorra[total_criado].valor = valor_atual;
            total_criado++;
        }
    }

    /* Pocoes: Copas de 2 ate 14 (As) -> 13 cartas */
    for (valor_atual = 2; valor_atual <= 14; valor_atual++) {
        masmorra[total_criado].naipe = NAIPE_COPAS;
        masmorra[total_criado].tipo  = TIPO_POCAO;
        masmorra[total_criado].valor = valor_atual;
        total_criado++;
    }

    /* Armas: Ouros de 2 ate 14 (As) -> 13 cartas */
    for (valor_atual = 2; valor_atual <= 14; valor_atual++) {
        masmorra[total_criado].naipe = NAIPE_OUROS;
        masmorra[total_criado].tipo  = TIPO_ARMA;
        masmorra[total_criado].valor = valor_atual;
        total_criado++;
    }

    return total_criado;
}

static void embaralhar_baralho(struct Carta *masmorra, int total_cartas) {
    int indice_atual;
    int indice_sorteado;
    struct Carta carta_temporaria;

    for (indice_atual = total_cartas - 1; indice_atual > 0; indice_atual--) {
        indice_sorteado = rand() % (indice_atual + 1);
        carta_temporaria = masmorra[indice_atual];
        masmorra[indice_atual] = masmorra[indice_sorteado];
        masmorra[indice_sorteado] = carta_temporaria;
    }
}

static void exibir_estado_e_sala(struct Jogador jogador, struct Carta *sala,
                                 int *espaco_ocupado, int cartas_na_masmorra,
                                 int cartas_descartadas) {
    int limite_monstro_arma;
    int i;

    if (jogador.ultimo_monstro_derrotado == SEM_LIMITE_MONSTRO) {
        limite_monstro_arma = 14;
    } else {
        limite_monstro_arma = jogador.ultimo_monstro_derrotado - 1;
    }

    printf("\n---------------------------------------------------------\n");
    printf("VIDA: %d/%d | ARMA EQUIPADA: %d (Ataca monstro ate: %d)\n",
           jogador.vida, VIDA_MAXIMA, jogador.arma_equipada, limite_monstro_arma);
    printf("Cartas restantes na Masmorra: %d | Descartadas: %d\n",
           cartas_na_masmorra, cartas_descartadas);
    printf("---------------------------------------------------------\n");
    printf("CARTAS NA SALA ATUAL:\n");

    for (i = 0; i < TAMANHO_SALA; i++) {
        printf(" [%d] ", i + 1);
        if (espaco_ocupado[i]) {
            imprimir_carta(sala[i]);
        } else {
            printf("(espaco vazio)");
        }
        printf("\n");
    }
}

static void enfrentar_monstro(struct Jogador *jogador, struct Carta monstro,
                              int *abandonou_partida) {
    int pode_usar_arma;
    int dano_usando_arma;
    int escolheu_usar_arma;
    char escolha_combate;

    pode_usar_arma = (jogador->arma_equipada > 0) &&
                     (monstro.valor < jogador->ultimo_monstro_derrotado);
    dano_usando_arma = monstro.valor - jogador->arma_equipada;
    escolheu_usar_arma = 0;

    if (dano_usando_arma < 0) {
        dano_usando_arma = 0;
    }

    if (pode_usar_arma) {
        printf("\nMonstro encontrado (Forca: %d).\n", monstro.valor);
        printf(" 1. Lutar desarmado (Recebe %d de dano)\n", monstro.valor);
        printf(" 2. Usar arma equipada (Recebe %d de dano)\n", dano_usando_arma);

        escolha_combate = ler_opcao_teclado("Escolha como lutar (1 ou 2): ");
        if (escolha_combate == 'x' || escolha_combate == '\0') {
            *abandonou_partida = 1;
            return;
        }
        if (escolha_combate == '2') {
            escolheu_usar_arma = 1;
        }
    }

    if (escolheu_usar_arma) {
        jogador->vida -= dano_usando_arma;
        jogador->ultimo_monstro_derrotado = monstro.valor;
        printf("\nVoce atacou com a arma e recebeu %d de dano!\n", dano_usando_arma);
    } else {
        jogador->vida -= monstro.valor;
        printf("\nVoce lutou desarmado e recebeu %d de dano!\n", monstro.valor);
    }
}

static int calcular_pontuacao_derrota(struct Jogador jogador,
                                      struct Carta *masmorra,
                                      int cartas_na_masmorra,
                                      struct Carta *sala,
                                      int *espaco_ocupado) {
    int soma_monstros_restantes;
    int i;

    soma_monstros_restantes = 0;

    for (i = 0; i < cartas_na_masmorra; i++) {
        if (masmorra[i].tipo == TIPO_MONSTRO) {
            soma_monstros_restantes += masmorra[i].valor;
        }
    }

    for (i = 0; i < TAMANHO_SALA; i++) {
        if (espaco_ocupado[i] && sala[i].tipo == TIPO_MONSTRO) {
            soma_monstros_restantes += sala[i].valor;
        }
    }

    return jogador.vida - soma_monstros_restantes;
}

static void jogar_partida(void) {
    struct Carta *masmorra;
    struct Carta *sala;
    int *espaco_ocupado;

    struct Carta carta_escolhida;
    struct Jogador jogador;

    int cartas_na_masmorra;
    int cartas_na_sala;
    int cartas_descartadas;
    int cartas_enfrentadas_no_turno;
    int pode_pular_sala;
    int posicao_escolhida;
    int pontuacao_final;
    int vida_anterior;
    int abandonou_partida;
    char opcao_digitada;
    int i;

    /* Alocacao dinamica de memoria usando apenas malloc */
    masmorra = (struct Carta *)malloc(TOTAL_CARTAS * sizeof(struct Carta));
    sala = (struct Carta *)malloc(TAMANHO_SALA * sizeof(struct Carta));
    espaco_ocupado = (int *)malloc(TAMANHO_SALA * sizeof(int));

    if (masmorra == NULL || sala == NULL || espaco_ocupado == NULL) {
        printf("\nErro: memoria insuficiente para iniciar o jogo.\n");
        free(masmorra);
        free(sala);
        free(espaco_ocupado);
        return;
    }

    /* Inicializacao manual do vetor espaco_ocupado (necessario ao usar malloc) */
    for (i = 0; i < TAMANHO_SALA; i++) {
        espaco_ocupado[i] = 0;
    }

    /* Inicializacao dos dados do jogador e da partida */
    jogador.vida = VIDA_MAXIMA;
    jogador.arma_equipada = 0;
    jogador.ultimo_monstro_derrotado = SEM_LIMITE_MONSTRO;
    jogador.usou_pocao_no_turno = 0;
    jogador.ultima_carta_jogada.naipe = 0;
    jogador.ultima_carta_jogada.tipo = 0;
    jogador.ultima_carta_jogada.valor = 0;

    cartas_na_sala = 0;
    cartas_descartadas = 0;
    cartas_enfrentadas_no_turno = 0;
    pode_pular_sala = 1;
    abandonou_partida = 0;

    cartas_na_masmorra = criar_baralho(masmorra);
    embaralhar_baralho(masmorra, cartas_na_masmorra);

    printf("\n=========================================================\n");
    printf("            INICIANDO NOVA PARTIDA DE SCOUNDREL          \n");
    printf("=========================================================\n");
    printf("(Digite 'x' a qualquer momento para voltar ao Menu)\n");

    while (jogador.vida > 0) {
        /* Preenche os espacos vazios da sala no inicio de cada turno */
        if (cartas_enfrentadas_no_turno == 0) {
            for (i = 0; i < TAMANHO_SALA && cartas_na_masmorra > 0; i++) {
                if (!espaco_ocupado[i]) {
                    sala[i] = masmorra[--cartas_na_masmorra];
                    espaco_ocupado[i] = 1;
                    cartas_na_sala++;
                }
            }
        }

        /* Verifica condicao de vitoria */
        if (cartas_na_sala == 0 && cartas_na_masmorra == 0) {
            printf("\n*** PARABENS! VOCE COMPLETOU TODA A MASMORRA! ***\n");
            pontuacao_final = jogador.vida;
            if (jogador.vida == VIDA_MAXIMA &&
                jogador.ultima_carta_jogada.tipo == TIPO_POCAO) {
                pontuacao_final += jogador.ultima_carta_jogada.valor;
            }
            printf("Pontuacao Final: %d\n\n", pontuacao_final);

            free(masmorra);
            free(sala);
            free(espaco_ocupado);
            return;
        }

        exibir_estado_e_sala(jogador, sala, espaco_ocupado,
                             cartas_na_masmorra, cartas_descartadas);

        printf("\nO que deseja fazer?\n");
        printf(" (1-%d) Escolher uma carta da sala\n", TAMANHO_SALA);
        if (pode_pular_sala && cartas_enfrentadas_no_turno == 0 &&
            cartas_na_sala == TAMANHO_SALA) {
            printf(" (f)   Fugir/Pular esta sala\n");
        }
        printf(" (x)   Abandonar partida e voltar ao Menu\n");

        opcao_digitada = ler_opcao_teclado("\nSua escolha: ");

        if (opcao_digitada == 'x' || opcao_digitada == '\0') {
            printf("\nPartida abandonada.\n\n");
            free(masmorra);
            free(sala);
            free(espaco_ocupado);
            return;
        }

        /* Acao de pular a sala: envia as 4 cartas para o fundo do baralho */
        if (opcao_digitada == 'f' && pode_pular_sala &&
            cartas_enfrentadas_no_turno == 0 && cartas_na_sala == TAMANHO_SALA) {
            printf("\nVoce pulou a sala! As cartas voltaram para o fundo da Masmorra.\n");
            memmove(&masmorra[cartas_na_sala], &masmorra[0],
                    (size_t)cartas_na_masmorra * sizeof(struct Carta));
            memcpy(&masmorra[0], sala, (size_t)cartas_na_sala * sizeof(struct Carta));
            cartas_na_masmorra += cartas_na_sala;

            for (i = 0; i < TAMANHO_SALA; i++) {
                espaco_ocupado[i] = 0;
            }
            cartas_na_sala = 0;
            pode_pular_sala = 0;
            continue;
        }

        if (opcao_digitada == 'f') {
            if (!pode_pular_sala) {
                printf("\nVoce nao pode pular duas salas seguidas!\n");
            } else {
                printf("\nVoce so pode pular a sala antes de enfrentar qualquer carta.\n");
            }
            continue;
        }

        posicao_escolhida = opcao_digitada - '1';
        if (posicao_escolhida < 0 || posicao_escolhida >= TAMANHO_SALA) {
            printf("\nOpcao invalida! Escolha um numero de 1 a %d.\n", TAMANHO_SALA);
            continue;
        }
        if (!espaco_ocupado[posicao_escolhida]) {
            printf("\nO espaco %d esta vazio! Escolha outra carta.\n", posicao_escolhida + 1);
            continue;
        }

        carta_escolhida = sala[posicao_escolhida];
        jogador.ultima_carta_jogada = carta_escolhida;

        if (carta_escolhida.tipo == TIPO_POCAO) {
            if (!jogador.usou_pocao_no_turno) {
                vida_anterior = jogador.vida;
                jogador.vida += carta_escolhida.valor;
                if (jogador.vida > VIDA_MAXIMA) {
                    jogador.vida = VIDA_MAXIMA;
                }
                jogador.usou_pocao_no_turno = 1;
                printf("\nPocao bebida! Voce recuperou %d de vida. Vida atual: %d\n",
                       jogador.vida - vida_anterior, jogador.vida);
            } else {
                printf("\nVoce ja bebeu uma pocao neste turno! Esta foi descartada sem efeito.\n");
            }
            cartas_descartadas++;
        }
        else if (carta_escolhida.tipo == TIPO_ARMA) {
            jogador.arma_equipada = carta_escolhida.valor;
            jogador.ultimo_monstro_derrotado = SEM_LIMITE_MONSTRO;
            printf("\nNova arma equipada! Poder de ataque: %d\n", jogador.arma_equipada);
            cartas_descartadas++;
        }
        else {
            enfrentar_monstro(&jogador, carta_escolhida, &abandonou_partida);
            if (abandonou_partida) {
                printf("\nPartida abandonada.\n\n");
                free(masmorra);
                free(sala);
                free(espaco_ocupado);
                return;
            }
            cartas_descartadas++;
        }

        espaco_ocupado[posicao_escolhida] = 0;
        cartas_na_sala--;
        cartas_enfrentadas_no_turno++;

        if (cartas_enfrentadas_no_turno == 3) {
            cartas_enfrentadas_no_turno = 0;
            jogador.usou_pocao_no_turno = 0;
            pode_pular_sala = 1;
            printf("\nTurno concluido! A carta restante permanece para a proxima sala.\n");
        }
    }

    /* Fim de jogo por derrota */
    printf("\n=========================================================\n");
    printf("                       VOCE MORREU!                      \n");
    printf("=========================================================\n");

    pontuacao_final = calcular_pontuacao_derrota(jogador, masmorra,
                                                 cartas_na_masmorra, sala,
                                                 espaco_ocupado);
    printf("Pontuacao Final: %d\n\n", pontuacao_final);

    /* Liberacao da memoria alocada dinamicamente */
    free(masmorra);
    free(sala);
    free(espaco_ocupado);
}
