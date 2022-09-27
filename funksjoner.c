char* flaggBinary(char id)
{
    char c = id;

    char* bits = (char*) malloc(9 * sizeof(char));
    bits[8] = '\0';

    int i;
    for (i = 7; i >= 0; i--)
    {
        if ((c & (1 << i)) != 0){
            bits[7-i] = '1';
        } else {
            bits[7-i] = '0';
        } 
    }
    return bits;
}

char regneChecksum(int tekstLengde, char* jobbTekst)
{
    int i = 0;
    int verdi = 0;
    for (i = 0; i < tekstLengde; i++)
    {
        verdi += jobbTekst[i];
    }
    return verdi % 32;
}