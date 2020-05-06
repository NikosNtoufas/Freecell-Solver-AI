#include "C:\Users\Nikos\Desktop\freecell\general.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

struct card
{
    int icon;
    int rate;
};


struct stack
{
    struct card cards[13*2];//max size
	int size;
};


void push(struct stack *s,struct card item)
{
    struct card *p;

    p = &(s->cards);
    if(s->size < 13*2) //?????
    {
       p[s->size] = item;
        s->size++;
    }
    else
        printf("stack overflow");
}

struct card pop(struct stack *s)
{
    struct card *p;
    struct card item;


    p = &(s->cards);
    if(s->size>0)
    {
        item = p[s->size-1];
        s->size--;
        return item;
    }

    item.icon = -1;
    item.icon = -1;
    return item;
}

struct card getLastCard(struct stack *s)
{
   struct card *p;
    struct card item;


    p = &(s->cards);
    if(s->size>0)
    {
        item = p[s->size-1];
        return item;
    }

    item.icon = -1;
    item.icon = -1;
    return item;
}


//Return the card you want to move to another stack
//In case of combo returns the highest rate of the cards you want to move
struct card getHighestRateCard(struct stack *s,int combo)//?????????????????
{
    struct card *p;
    struct card item;


    p = &(s->cards);
    if(s->size>0)
    {
        int a = s->size;
        item = p[s->size-combo];
       // s->size--; //?????????
        return item;
    }

    item.icon = -1;
    item.icon = -1;
    return item;
};
