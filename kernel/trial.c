#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct info
{
	char suit[9];
	char face[5];
};

void DefineCard(struct info card[]);
void DealCard(struct info card[]);


int main()
{
	struct info card[52];
	DefineCard(card);
	DealCard(card);
	return 0;
}

void DefineCard(struct info card[])
{
	int i;
	for (i = 0; i < 52; i += 13)
		{
			strcpy(card[i].face, "A");
		}
	for (i = 1; i < 52; i += 13)
		{
			strcpy(card[i].face, "2");
		}
	for (i = 2; i < 52; i += 13)
		{
			strcpy(card[i].face, "3");
		}
	for (i = 3; i < 52; i += 13)
		{
			strcpy(card[i].face, "4");
		}
	for (i = 4; i < 52; i += 13)
		{
			strcpy(card[i].face, "5");
		}
	for (i = 5; i < 52; i += 13)
		{
			strcpy(card[i].face, "6");
		}
	for (i = 6; i < 52; i += 13)
		{
			strcpy(card[i].face, "7");
		}
	for (i = 7; i < 52; i += 13)
		{
			strcpy(card[i].face, "8");
		}
	for (i = 8; i < 52; i += 13)
		{
			strcpy(card[i].face, "9");
		}
	for (i = 9; i < 52; i += 13)
		{
			strcpy(card[i].face, "10");
		}
	for (i = 10; i < 52; i += 13)
		{
			strcpy(card[i].face, "Jack");
		}
	for (i = 11; i < 52; i += 13)
		{
			strcpy(card[i].face, "Queen");
		}
	for (i = 12; i < 52; i += 13)
		{
			strcpy(card[i].face, "King");
		}
	for (i = 0; i < 52; i++)
		{
			if (i <= 12)
				strcpy(card[i].suit, "Spades");
			if (i <= 25 && i >= 13)
				strcpy(card[i].suit, "Hearts");
			if (i <= 38 && i >= 26)
				strcpy(card[i].suit, "Clubs");
			if (i >= 39)
				strcpy(card[i].suit, "Diamonds");
		}
    switch (expression)
    {
    case /* constant-expression */:
        /* code */
        break;
    
    default:
        break;
    }
}

void DealCard(struct info card[])
{
	int i;
	for (i = 0; i < 52; i++)
		{
			printf("%s %s\n", card[i].suit, card[i].face);
		}
}