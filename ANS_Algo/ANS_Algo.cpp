#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//
// Paramètres pour le range coding
//
#define TOP_VALUE 0xFFFFFFFFu
#define MIN_RANGE 0x01000000u   // Seuil pour normaliser (2^24)

//
// Nombre de symboles : 256 octets + 1 symbole EOF
//
#define SYMBOLS 257

//
// Fonction compressFileRangeCoding : compresse le fichier d'entrée via le range coding
//
void compressFileRangeCoding(const char* inputFileName, const char* outputFileName) {
    FILE* in = fopen(inputFileName, "rb");
    if (!in) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", inputFileName);
        return;
    }

    // Calcul de la table de fréquences pour chaque symbole (0..255)
    // On ajoute un symbole spécial EOF (indice 256) avec fréquence 1
    int freq[SYMBOLS] = { 0 };
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        freq[ch]++;
    }
    freq[256] = 1;

    // Construction de la table cumulative : cum[0] = 0, et pour i de 0 à SYMBOLS-1
    int cum[SYMBOLS + 1];
    cum[0] = 0;
    for (int i = 0; i < SYMBOLS; i++) {
        cum[i + 1] = cum[i] + freq[i];
    }
    int total = cum[SYMBOLS];

    // Ouverture du fichier de sortie en mode binaire et écriture de la table de fréquences
    FILE* out = fopen(outputFileName, "wb");
    if (!out) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s pour l'écriture\n", outputFileName);
        fclose(in);
        return;
    }
    fwrite(freq, sizeof(int), SYMBOLS, out);

    // Retour au début du fichier d'entrée
    fseek(in, 0, SEEK_SET);

    // Initialisation des variables du range coder
    unsigned int low = 0;
    unsigned int range = TOP_VALUE;

    // Encodage des symboles du fichier d'entrée
    while ((ch = fgetc(in)) != EOF) {
        // Calcul de la taille de sous-intervalle correspondant à un point unitaire
        unsigned int r = range / total;
        // Mise à jour de l'intervalle courant
        low += r * cum[ch];
        range = r * freq[ch];

        // Normalisation : tant que la plage est trop petite, on décale et on écrit le byte de poids fort de low
        while (range < MIN_RANGE) {
            fputc((low >> 24) & 0xFF, out);
            low <<= 8;
            range <<= 8;
        }
    }

    // Encodage du symbole EOF (indice 256)
    {
        unsigned int r = range / total;
        low += r * cum[256];
        range = r * freq[256];
        while (range < MIN_RANGE) {
            fputc((low >> 24) & 0xFF, out);
            low <<= 8;
            range <<= 8;
        }
    }

    // Finalisation : on vide les derniers octets de low (4 octets)
    for (int i = 0; i < 4; i++) {
        fputc((low >> 24) & 0xFF, out);
        low <<= 8;
    }

    fclose(in);
    fclose(out);
}

//
// Fonction main : boucle sur les fichiers "0.txt", "1.txt", etc.
// Mesure le temps de compression pour chacun et affiche un message récapitulatif.
//
int main() {
    int i = 1;
    char inputFileName[100];
    char outputFileName[100];

    while (1) {
        sprintf(inputFileName, "C:/Users/donde/source/test/random/%dbook.txt", i);
        FILE* testFile = fopen(inputFileName, "rb");
        if (!testFile) {
            break; // Arrête la boucle si le fichier n'existe pas
        }
        fclose(testFile);

        sprintf(outputFileName, "C:/Users/donde/source/test/random/%dbook.range", i);

        clock_t start = clock();
        compressFileRangeCoding(inputFileName, outputFileName);
        clock_t end = clock();
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Compression du fichier %s en %s terminee en %.3f secondes.\n",
            inputFileName, outputFileName, time_spent);

        if (i++ >= 1)
            break;
    }

    if (i == 0) {
        printf("Aucun fichier a compresser.\n");
    }

    return 0;
}
