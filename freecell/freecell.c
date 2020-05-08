
//you can read the rules of the game here->https://cardgames.io/freecell/#rules
/*input file example(Ν=13):
----------------------
C3 C9 D1 D6 D10 C1 D12
C8 S2 C5 D4 H1 S12 C6
D8 D3 C12 D9 S6 H5 C0
C11 C2 S0 S9 H3 H10 H11
S3 S8 D11 C7 S7 S1
S5 C4 H12 D5 C10 H8
D0 H7 S4 H4 H0 D2
D7 H6 H9 S11 S10 H2
----------------------
***** Letters define the suite of the cards and numbers define the rank*****
***** We have total N*4 cards, in 8 stacks.
----------------------------------------------------------------------------
*/
#include "structures.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

//------------------------------------

// Constants denoting the four ratings
#define HEARTS 0
#define DIAMONDS 1
#define SPADES 2
#define CLUBS 3

// Constants denoting the four algorithms
#define breadth 1
#define depth	2
#define best	3
#define astar	4

//Constants of the game
#define numberOfStacks 8
#define numberOfFreecells 4
#define numberOfFoundations 4

//time variables
clock_t t1;					// Start time of the search algorithm
clock_t t2;					// End time of the search algorithm
#define TIMEOUT		300	// Program terminates after TIMOUT secs


int solutionLength=0;	// The lenght of the solution table.
int *solution;			// Pointer to a dynamic table with the moves of the solution.
int N;                  //Number of max ranking
struct treeNode
{
    struct stack Tableau[numberOfStacks];           // The eight piles that make up the main table.
    struct card Freecells[numberOfFreecells];       // The four piles in the upper left corner. On each pile you can put one card.
    int FoundatationRates[numberOfFoundations];     // Each table store an integer with Foundation's last card's rate.Empty Foundations have -1.
	float h;						                // the value of the heuristic function for this node
	int g;						                // the depth of this node from the root of the search tree
	float f;						                // f=0 or f=h or f=h+g, depending on the search algorithm used.
    struct stack cardsMoved;                        //contains the cards moved in the current move
    int moveTo;                                     //0-7 for stacks,8 for free cell,9 for foundations
	struct treeNode *parent;	                    // pointer to the parent node (NULL for the root).
    struct child *firstChild;                       //pointer to the first child of the node
};

struct child
{
    struct treeNode *node;
    struct child *next;
};
// A node of the frontier. Frontier is kept as a float-linked list,
// for efficiency reasons for the breadth-first search algorithm.
struct frontierNode
{
	struct treeNode *n;				// pointer to a search-tree node
	struct frontierNode *previous;		// pointer to the previous frontier node
	struct frontierNode *next;			// pointer to the next frontier node
};

struct frontierNode *frontierHead=NULL;	// The one end of the frontier
struct frontierNode *frontierTale=NULL;	// The other end of the frontier
struct frontierNode *treeNodesHead=NULL;	// The other end of the frontier


// Function that displays a message in case of wrong input parameters.
void WrongInputMessage()
{
	printf("frecell <method> <input-file> <output-file>\n\n");
	printf("where: ");
	printf("<method> = breadth|depth|best|astar \n");
	printf("<input-file> is a file containing a freecell problem description with max value:%d.\n",13);
	printf("<output-file> is the file where the solution will be written.\n");
}


// Reading input algorithm.
int GetMethod(char* s)
{
    stringToLower(s);
	if (strcmp(s,"breadth")==0)
		return  breadth;
	else if (strcmp(s,"depth")==0)
		return depth;
	else if (strcmp(s,"best")==0)
		return best;
	else if (strcmp(s,"astar")==0)
		return astar;
	else
		return -1;
}

//Read input file and initialize all Tableau stacks
int ReadInputFromFile(char* filename, struct stack *allStacks,int size)
{
	FILE *fin;
    char c = malloc(1000 * sizeof(char));
    struct card tempCard;
	fin=fopen(filename, "r");
	if (fin==NULL)
	{
        printf("Cannot open file %s. Program terminates.\n",filename);
		return -1;
	}


    int stackCounter = 0;
    do
    {
        c = (char)fgetc(fin);
        tempCard.icon=-1;
        tempCard.rate=-1;
        while(c!=' ' && c!=EOF && c!='\n')
        {
            if(c=='C')
            {
                tempCard.icon = CLUBS;
            }
            else if(c=='H')
            {
                tempCard.icon = HEARTS;
            }
            else if(c=='D')
            {
                tempCard.icon = DIAMONDS;
            }
            else if(c=='S')
            {
               tempCard.icon = SPADES;
            }
            else
            {
                if(tempCard.rate==-1)
                {
                    tempCard.rate = c - '0';
                }
                else
                {
                    tempCard.rate = tempCard.rate*10 + (c - '0');
                }


            }
            c = (char)fgetc(fin);


        }

        if(c == '\n')
        {
            push(&allStacks[stackCounter],tempCard);
            stackCounter++;
        }
        else if(c == ' ')
        {
            push(&allStacks[stackCounter],tempCard);
        }
    } while(c != EOF && stackCounter < size);



	fclose(fin);
    return 1;
}

//verify input and define N
//return -1 if the input file is not in correct format
int verifyInput(struct stack *allStacks)
{
    int i,j;
    int max[numberOfFoundations];

    for(i=0;i<numberOfFoundations;i++)
        max[i]=-1;

    for(i=0;i<numberOfStacks; i++)
    {
        for(j=0; j<allStacks[i].size; j++)
        {
            if(allStacks[i].cards[j].rate > max[allStacks[i].cards[j].icon])
                max[allStacks[i].cards[j].icon] = allStacks[i].cards[j].rate;
        }
    }

    int tempRate = max[0];
    for(i=1;i<numberOfFoundations;i++)
    {
        if(max[i]!=tempRate)
            return -1; //each stack has different N
    }

    if(tempRate==-1 || tempRate>13)
        return -1; //wrong N in input file
    else
    {
        N=tempRate+1;
        return 1;
    }
}

//save the current move
//in this round a card has moved to a Foundation
void SaveMoveFoundation(struct treeNode *node,struct card tempCard)
{
    node->cardsMoved.size = 0;
    push(&node->cardsMoved,tempCard);
    node->moveTo = 9; //move to Foundation
}

//save the current move
//in this round a card has moved to a free cell
void SaveMoveFreecell(struct treeNode *node,struct card tempCard)
{
    node->cardsMoved.size = 0;
    push(&node->cardsMoved,tempCard);
    node->moveTo = 8;
}

//save the current move
//in this round one or more cards have moved to another stack
void SaveMoveAnotherStack(struct treeNode *node,int index,struct stack tempStack)
{
    node->cardsMoved.size = 0;
    while(tempStack.size>0)
    {
        push(&node->cardsMoved,pop(&tempStack));
    }
    node->moveTo = index;
}

// This function checks whether a node in the search tree
// This is a moderate way to detect loops in the search.
// Inputs:
//		struct treeNode *newNode	: A search tree node (usually a new one)
// Output:
//		1 --> No coincidence with any predecessor
//		0 --> Loop detection
int checkWithParents(struct treeNode *newNode)
{
    //check all nodes
    int k;
	k = checkAllNodes(newNode);

	return k;

	//instead of checking all nodes of the tree
	//you can check only the parents
	//or all its parents with their children

	/*struct treeNode *parent=newNode->parent;

	while (parent!=NULL)
	{

	    if (AllStacksEquals(newNode,parent))
			return 0;
        struct child *tempChild;
        int c = 1;
        tempChild = parent->firstChild;
        while(tempChild!=NULL)
        {
            if(AllStacksEquals(newNode,tempChild->node))
                return 0;
            tempChild=tempChild->next;
        }
		parent=parent->parent;
	}


	return 1;*/

}

//check all nodes to detect loops
int checkAllNodes(struct treeNode *newNode)
{
    struct frontierNode *tempChild;
    tempChild = treeNodesHead;
    while(tempChild!=NULL)
    {

        //displayGameTable(tempChild->n);
        if(AllStacksEquals(newNode,tempChild->n))
            return 0;
        tempChild=tempChild->next;
    }
    return 1;
}

// This function checks whether two nodes are equals
//		1 --> The stacks are equal
//		0 --> The stacks are not equal
int AllStacksEquals(struct treeNode *new_node,struct treeNode *parent)
{
    int flag = 0;
    int numberOfEmptyStacksNew=0;
    int numberOfEmptyStacksParent=0;

    //check foundations
    for(int i=0;i<numberOfFoundations;i++)
    {
        if(new_node->FoundatationRates[i]!=parent->FoundatationRates[i])
            return 0; //not equals
    }


    //in case of empty stacks , check the stacks with same sizes
    for(int i=0; i<numberOfStacks; i++)
    {
        if(new_node->Tableau[i].size==0)
            numberOfEmptyStacksNew++;
        if(parent->Tableau[i].size==0)
            numberOfEmptyStacksParent++;
    }



    if(numberOfEmptyStacksNew!=numberOfEmptyStacksParent)
        return 0; //two nodes are not equals, they haven't the same number of empty stacks


    //Match every parent stacks.
    //If all parent Tableau stacks have one equal new node stack, the two nodes are equals
    //0 available / 1 not available
    int availableParentStacks[numberOfStacks];
    //-initialize
    for(int k=0; k<numberOfStacks;k++)
    {
        availableParentStacks[k] = 0;
    }



    for(int nIndex=0; nIndex<numberOfStacks; nIndex++)
    {
        for(int pIndex=0; pIndex<numberOfStacks; pIndex++)
        {
            if(availableParentStacks[pIndex] == 1) //parent stack already matched
                continue;

            //Two stacks can be equals only if they have the same size
            if(availableParentStacks[pIndex]==0 && new_node->Tableau[nIndex].size==parent->Tableau[pIndex].size)
            {
                //two eguals empty stack
                if(new_node->Tableau[nIndex].size==0)
                {
                    availableParentStacks[pIndex] = 1;
                    break;
                }
                else
                {
                    int equals = 1;
                    //check all the cards
                    for(int j=0; j<new_node->Tableau[nIndex].size; j++)
                    {
                        struct card c1 = new_node->Tableau[nIndex].cards[j];
                        struct card c2 = parent->Tableau[pIndex].cards[j];

                        if(c1.icon!=c2.icon || c1.rate!=c2.rate)
                        {
                            equals = 0;
                            break;
                        }
                    }
                    if(equals)
                    {
                        availableParentStacks[pIndex]=1; // nIndex and pIndex stacks are equals
                        break;
                    }

                }
            }
        }
    }

    for(int i=0;i<numberOfStacks;i++)
    {
        if(availableParentStacks[i]==0)
            return 0; //not equals
    }
    return 1;

}

//return the number of the cards that have the correct position in the stack
//NOT USED
int getNumWellPlaced(struct treeNode *currNode)
{
    int j,i;
    int count =0;
    for(i=0; i<numberOfStacks; i++)
    {
        int size = currNode->Tableau[i].size;

        int temp[size];
        for(int j=0; j<currNode->Tableau[i].size; j++)
        {
            temp[j]= currNode->Tableau[i].cards[j].rate;
        }

        //sort stack
        countSort(temp,size,N);

        for( j=0; j<currNode->Tableau[i].size; j++)
        {
            if(temp[j]==currNode->Tableau[i].cards[j].rate)
                count++;
        }
    }

    return count;

}



//count sort
void countSort(int arr[], int n, int k)
{
	// create an integer array of size n to store sorted array
	int output[n];

	// create an integer array of size k, initialized by all zero
	int freq[k];
	memset(freq, 0, sizeof(freq));

	// using value of integer in the input array as index,
	// store count of each integer in freq[] array
	for (int i = 0; i < n; i++)
		freq[arr[i]]++;

	// calculate the starting index for each integer
	int total = 0;
	for (int i = 0; i < k; i++)
	{
		int oldCount = freq[i];
		freq[i] = total;
		total += oldCount;
	}

	// copy to output array, preserving order of inputs with equal keys
	for (int i = 0; i < n; i++)
	{
		output[freq[arr[i]]] = arr[i];
		freq[arr[i]]++;
	}

	// copy the output array back to the input array
	for (int i = 0; i < n; i++)
		arr[i] = output[i];
}

//returns 1 if all the cards are in the Foundations
//higher values means better positions for cards
//returns the distance from the top of the stack of the next cards that can be moved to foundation
float GetTargetCardsDistance(struct treeNode *currNode)
{

    int j,i;
    float distance[numberOfFoundations];
    int min[numberOfFoundations];
    int k = N;
    for(int f=0; f<numberOfFoundations;f++)
    {
        min[f]=currNode->FoundatationRates[f]+1;
        if(min[f]==N)
            distance[f] = 0.25;
        else
            distance[f] = 0.24; //if the card cannot be found in stacks, it is on freecell ->distance 0.24 one move from foundation
    }


    for(i=0; i<numberOfStacks; i++)
    {
        int size = currNode->Tableau[i].size;

        for(j=0; j<currNode->Tableau[i].size; j++)
        {
            if(min[currNode->Tableau[i].cards[j].icon]== currNode->Tableau[i].cards[j].rate)
            {
                float temp = ((float)size-(float)j)/100.0;
                distance[currNode->Tableau[i].cards[j].icon] = 0.25 - temp;
            }

        }
    }

    float count=0.0;
    for(i=0;i<numberOfFoundations;i++)
    {
        count = count + distance[i];
    }

    return count;

}

float heuristic(struct treeNode *currNode)
{
    float weight= 1.0;
    //weight = 2.0;
    //heuristic weight , in case of a* algorithm If you increase the weight
    // you reduce the time and possibly you will find a better solution than the best algorithm
    //a* is optimum with weight = 1 only
    float countFoundationCards=0;
    for(int i=0; i<numberOfFoundations;i++)
    {
        countFoundationCards+=currNode->FoundatationRates[i]+1;
    }

    float cardsLeft = (N*4+1)-countFoundationCards;
    float distance = GetTargetCardsDistance(currNode);
    return (cardsLeft-distance)*weight;
}


// This function initializes the search, i.e. it creates the root node of the search tree
// and the first node of the frontier.
void InitializeSearch(struct stack t[numberOfStacks], int method)
{
	struct treeNode *root=NULL;	// the root of the search tree.
	int stackSize=0;
	int i,j;

	// Initialize search tree
	root=(struct treeNode*) malloc(sizeof(struct treeNode));
	root->parent=NULL;
	root->firstChild = NULL;

	//initialize free cells / icon =-1 , rate = -1 ---> empty card
	for(i=0;i<numberOfFreecells;i++)
    {
        root->Freecells[i].icon = -1;
        root->Freecells[i].rate = -1;
    }

    //initialize Foundations
    for(i=0;i<numberOfFoundations;i++)
    {
        root->FoundatationRates[i] = -1; // empty Foundation
    }

	//copy Tableau stacks to treeNode
	for(i=0;i<numberOfStacks;i++)
	{
         stackSize = t[i].size;
            for(j=0;j<stackSize;j++)
                root->Tableau[i].cards[j] = t[i].cards[j];
        root->Tableau[i].size = t[i].size;
	}



	root->g=0;
	root->h=heuristic(root);
	if (method==best)
		root->f=root->h;
	else if (method==astar)
		root->f=(float)root->g+root->h;
	else
		root->f=0.0;

	// Initialize frontier
	AddFrontierFront(root);
	addToAllNodes(root);
}

//add child to parent node
//NOT USED!!!! Checking for loops in all nodes so this functionality is not used
int addChild(struct treeNode *parentNode,struct treeNode *newNode)
{

	// Creating the new child node
	struct child *newChild=(struct child*)
                                malloc(sizeof(struct child));
	if (newChild==NULL)
		return -1;

	newChild->node=newNode;
	newChild->next=NULL;

	if (parentNode->firstChild==NULL)
	{
		parentNode->firstChild =newChild;

	}
	else
    {
        int flag = 1;
        struct child *pt = (struct child*)
                                malloc(sizeof(struct child));
        if (pt==NULL)
            return -1;

        pt = parentNode->firstChild;
        while(flag)
        {

            if(pt->next==NULL)
            {
                pt->next = newChild;
                flag = 0;
            }
            else
            {
                pt = pt->next;
            }
        }
    }


	return 0;
}

//Saving new Node to all nodes structure
//Use this functionality to check for loops in the next nodes
int addToAllNodes(struct treeNode *node)
{
	// Creating the new frontier node
	struct frontierNode *new_frontierNode=(struct frontierNode*)
                                malloc(sizeof(struct frontierNode));
	if (new_frontierNode==NULL)
		return -1;

	new_frontierNode->n=node;
	new_frontierNode->previous=NULL;
	new_frontierNode->next=treeNodesHead;

	if (treeNodesHead==NULL)
	{
		treeNodesHead=new_frontierNode;
	}
	else
	{
		treeNodesHead->previous=new_frontierNode;
		treeNodesHead=new_frontierNode;
	}

	return 0;
}

// This function adds a pointer to a new leaf search-tree node at the front of the frontier.
// This function is called by the depth-first search algorithm.
// Inputs:
//		struct treeNode *node	: A (leaf) search-tree node.
// Output:
//		0 --> The new frontier node has been added successfully.
//		-1 --> Memory problem when inserting the new frontier node .
int AddFrontierFront(struct treeNode *node)
{
	// Creating the new frontier node
	struct frontierNode *new_frontierNode=(struct frontierNode*)
                                malloc(sizeof(struct frontierNode));
	if (new_frontierNode==NULL)
		return -1;

	new_frontierNode->n=node;
	new_frontierNode->previous=NULL;
	new_frontierNode->next=frontierHead;

	if (frontierHead==NULL)
	{
		frontierHead=new_frontierNode;
		frontierTale=new_frontierNode;
	}
	else
	{
		frontierHead->previous=new_frontierNode;
		frontierHead=new_frontierNode;
	}

	return 0;
}

// This function adds a pointer to a new leaf search-tree node at the back of the frontier.
// This function is called by the breadth-first search algorithm.
// Inputs:
//		struct treeNode *node	: A (leaf) search-tree node.
// Output:
//		0 --> The new frontier node has been added successfully.
//		-1 --> Memory problem when inserting the new frontier node .
int AddFrontierBack(struct treeNode *node)
{
	// Creating the new frontier node
	struct frontierNode *new_frontierNode=(struct frontierNode*) malloc(sizeof(struct frontierNode));
	if (new_frontierNode==NULL)
		return -1;

	new_frontierNode->n=node;
	new_frontierNode->next=NULL;
	new_frontierNode->previous=frontierTale;

	if (frontierTale==NULL)
	{
		frontierHead=new_frontierNode;
		frontierTale=new_frontierNode;
	}
	else
	{
		frontierTale->next=new_frontierNode;
		frontierTale=new_frontierNode;
	}


	return 0;
}

// This function adds a pointer to a new leaf search-tree node within the frontier.
// The frontier is always kept in increasing order wrt the f values of the corresponding
// search-tree nodes. The new frontier node is inserted in order.
// This function is called by the heuristic search algorithm.
// Inputs:
//		struct treeNode *node	: A (leaf) search-tree node.
// Output:
//		0 --> The new frontier node has been added successfully.
//		-1 --> Memory problem when inserting the new frontier node .
int AddFrontierInOrder(struct treeNode *node)
{
	// Creating the new frontier node
	struct frontierNode *new_frontierNode=(struct frontierNode*)
                malloc(sizeof(struct frontierNode));
	if (new_frontierNode==NULL)
		return -1;

	new_frontierNode->n=node;
	new_frontierNode->previous=NULL;
	new_frontierNode->next=NULL;

	if (frontierHead==NULL)
	{
		frontierHead=new_frontierNode;
		frontierTale=new_frontierNode;
	}
	else
	{
		struct frontierNode *pt;
		pt=frontierHead;

		// Search in the frontier for the first node that corresponds to either a larger f value
		// or to an equal f value but larger h value
		// Note that for the best first search algorithm, f and h values coincide.
		while (pt!=NULL && (pt->n->f<node->f || (pt->n->f==node->f && pt->n->h<node->h)))
        {
            float a = pt->n->f;
            float b = node->f;
           // float ah =pt->n->g;
           // float bh =node->g;
            pt=pt->next;
        }


		if (pt!=NULL)
		{
			// new_frontierNode is inserted before pt .
			if (pt->previous!=NULL)
			{
				pt->previous->next=new_frontierNode;
				new_frontierNode->next=pt;
				new_frontierNode->previous=pt->previous;
				pt->previous=new_frontierNode;
			}
			else
			{
				// In this case, new_frontierNode becomes the first node of the frontier.
				new_frontierNode->next=pt;
				pt->previous=new_frontierNode;
				frontierHead=new_frontierNode;
			}
		}
		else
		{
			// if pt==NULL, new_frontierNode is inserted at the back of the frontier
			frontierTale->next=new_frontierNode;
			new_frontierNode->previous=frontierTale;
			frontierTale=new_frontierNode;
		}
	}

//	display_puzzle(node);


	return 0;
}


// This function implements at the higest level the search algorithms.
// The various search algorithms differ only in the way the insert
// new nodes into the frontier, so most of the code is commmon for all algorithms.
// Inputs:
//		Nothing, except for the global variables root, frontierHead and frontierTale.
// Output:
//		NULL --> The problem cannot be solved
//		struct treeNode*	: A pointer to a search-tree leaf node that corresponds to a solution.


struct treeNode *search(int method)
{
    clock_t t;
	int err;
	struct frontierNode *temp_frontierNode;
	struct treeNode *current_node;

	while (frontierHead!=NULL)
	{
		t=clock();
		if (t-t1>CLOCKS_PER_SEC*TIMEOUT)
		{
			printf("Timeout\n");
			return NULL;
		}

		// Extract the first node from the frontier
		current_node=frontierHead->n;

        //displayGameTable(current_node);

		if (gameWon(current_node))
        {
            return current_node;
        }


		// Delete the first node of the frontier
		temp_frontierNode=frontierHead;
		frontierHead=frontierHead->next;
		free(temp_frontierNode);
		if (frontierHead==NULL)
			frontierTale=NULL;
		else
			frontierHead->previous=NULL;

		if(current_node->g<=N*100 || method!= depth) //limit depth for depth algorithm
            err=FindChildren(current_node, method);


		if (err<0)
        {
            printf("Memory exhausted while creating new frontier node. Search is terminated...\n");
			return NULL;
        }
	}

	return NULL;

}


// This function expands a leaf-node of the search tree.
// A leaf-node may have childs for stack moves or from free cell moves
// In case no child exists (due to loop-detections), the table is not created
// and a 'higher-level' NULL indicates this situation.
// Inputs:
//		struct treeNode *current_node	: A leaf-node of the search tree.
// Output:
//		The same leaf-node expanded with pointers to its children (if any).
int FindChildren(struct treeNode *current_node, int method)
{

	// For each stack find the children
    for(int i=0;i<numberOfStacks;i++)
    {
        if(current_node->Tableau[i].size>0) // find children for Tableau stacks
        {
            int highestCombo =
             getNumberOfComboCardsInStack(current_node->Tableau[i],getHighestComboAllowed(current_node->Tableau,current_node->Freecells));
            //if stack has cards find children
            if(highestCombo>0)
            {
                int err = findChilderForAStack(current_node,i,method,highestCombo);
                if (err<0)
                    return -1;
            }

        }
    }

    //for each free cell find the children
    for(int i=0;i<numberOfFreecells;i++)
    {


        struct card frecellCard = current_node->Freecells[i];

        if(frecellCard.rate>-1)
        {
            //freecell has card
            int err = findChilderForAFreecell(current_node,i,method);
            if(err<0)
                return -1;
        }

    }


	return 1;
}

//new node added
int addNewNode(struct treeNode *child,int method)
{
    // Computing the heuristic value if needed only
            if(method== best || method == astar)
                child->h=heuristic(child);
            else
                child->h=0.0;

			if (method==best)
				child->f=child->h;
			else if (method==astar)
				child->f=(float)child->g+child->h;
			else
				child->f=0.0;
            child->firstChild =NULL;

             int errorChilds = addToAllNodes(child);
            if(errorChilds<0)
                return -1;

            //add child to parent node childs ,NOT USED!
           /* int errorChilds = addChild(current_node,child);
            if(errorChilds<0)
                return -1;*/

            int err=0;
            if (method==depth)
				err=AddFrontierFront(child);
			else if (method==breadth)
				err=AddFrontierBack(child);
			else if (method==best || method==astar)
				err=AddFrontierInOrder(child);
			if (err<0)
                return -1;
}

//Find the children for the cards of the stacks
int findChilderForAStack(struct treeNode *current_node,int index,int method,int comboNum)
{
    struct card c = getLastCard(&current_node->Tableau[index]);
    int addToFoundation = allowAddCardToFoundation(c,current_node->FoundatationRates);

    //add card to Foundation
    if(addToFoundation)
    {
		// Initializing the new child
		struct treeNode *child=(struct treeNode*) malloc(sizeof(struct treeNode));

		if (child==NULL) return -1;

		child->f = current_node->f;
		child->h = current_node->h;
		for(int i=0;i<numberOfFoundations;i++)
        {
            child->FoundatationRates[i] = current_node->FoundatationRates[i];
		}
		for(int i=0; i<numberOfFreecells;i++)
        {
            child->Freecells[i]=current_node->Freecells[i];
        }
        for(int i=0; i<numberOfStacks;i++)
        {
             child->Tableau[i].size = current_node->Tableau[i].size;
            for(int j=0;j<child->Tableau[i].size;j++)
            {
                child->Tableau[i].cards[j]= current_node->Tableau[i].cards[j];
            }

        }
		child->parent=current_node;
		child->g=current_node->g+1;	 	// The depth of the new child
		// remove card from the stack
		struct card tempCard = pop(&child->Tableau[index]);
		// add card to Foundation
		child->FoundatationRates[tempCard.icon]= tempCard.rate;

		//save the moved cards
        SaveMoveFoundation(child,tempCard);

		// Check for loops
		if (!checkWithParents(child))
			// In case of loop detection, the child is deleted
			free(child);
		else
		{
		    int error;
		    error = addNewNode(child,method);
		    if(error<0)
                return -1;
		}

    }



    //every combo has different available stacks to remove
    for(int combo=1; combo<=comboNum; combo++)
    {
        int addedToEmptyStack = 0; //move to empty stack only once

        //check every other stack for this combo
        for(int i=0; i<numberOfStacks;i++)
        {
            //don't check current stack and don't move last card to other empty stack
            if(i==index || (current_node->Tableau[index].size==1 && current_node->Tableau[i].size==0))
                continue;

            int addToStack = allowAddCardToCurrentStack(current_node->Tableau[index],current_node->Tableau[i],combo);
            if(current_node->Tableau[i].size==0)
            {
                addedToEmptyStack++;
                //move to empty stack only once
                if(addedToEmptyStack>=1)
                   addToStack=0;
            }
            if(addToStack)
            {
                // Initializing the new child
                struct treeNode *child=(struct treeNode*) malloc(sizeof(struct treeNode));

                if (child==NULL) return -1;

                for(int i=0;i<numberOfFoundations;i++)
                {
                    child->FoundatationRates[i] = current_node->FoundatationRates[i];
                }
                for(int i=0; i<numberOfFreecells;i++)
                {
                    child->Freecells[i]=current_node->Freecells[i];
                }
                for(int i=0; i<numberOfStacks;i++)
                {
                     child->Tableau[i].size = current_node->Tableau[i].size;
                    for(int j=0;j<child->Tableau[i].size;j++)
                    {
                        child->Tableau[i].cards[j]= current_node->Tableau[i].cards[j];
                    }

                }


                child->parent=current_node;
                child->g=current_node->g+1;	 	// The depth of the new child


                struct stack tempStack;
                tempStack.size = 0;

                // remove card from the stack
                for(int c=0;c<combo;c++)
                {
                    struct card tempCard = pop(&child->Tableau[index]);
                    push(&tempStack,tempCard);
                }

                //save move
                SaveMoveAnotherStack(child,i,tempStack);

                while(tempStack.size>0)
                {
                    push(&child->Tableau[i],pop(&tempStack));
                }

                // Check for loops
                if (!checkWithParents(child))
                    // In case of loop detection, the child is deleted
                    free(child);
                else
                {
                    int error;
                    error = addNewNode(child,method);
                    if(error<0)
                        return -1;

                }

            }
        }


        //single cards can be moved to freecells also
        if(combo==1 && !addToFoundation)
        {
            int freecell = availableFreecell(current_node->Freecells);
            if(freecell>-1)
            {
                // Initializing the new child
                struct treeNode *child=(struct treeNode*) malloc(sizeof(struct treeNode));

                if (child==NULL) return -1;

                for(int i=0;i<numberOfFoundations;i++)
                {
                    child->FoundatationRates[i] = current_node->FoundatationRates[i];
                }
                for(int i=0; i<numberOfFreecells;i++)
                {
                    child->Freecells[i]=current_node->Freecells[i];
                }
                for(int i=0; i<numberOfStacks;i++)
                {
                     child->Tableau[i].size = current_node->Tableau[i].size;
                    for(int j=0;j<child->Tableau[i].size;j++)
                    {
                        child->Tableau[i].cards[j]= current_node->Tableau[i].cards[j];
                    }

                }


                child->parent=current_node;
                child->g=current_node->g+1.0;	 	// The depth of the new child


                // remove card from the stack
                struct card tempCard = pop(&child->Tableau[index]);

                //add card to freecell
                child->Freecells[freecell] = tempCard;

                //save move
                SaveMoveFreecell(child,tempCard);

                //Check for loops
                if (!checkWithParents(child))
                    // In case of loop detection, the child is deleted
                    free(child);
                else
                {
                    int error;
                    error = addNewNode(child,method);
                    if(error<0)
                        return -1;

                }
            }

        }
    }


    return 1;
}

//Find all children for the cards of free cells
int findChilderForAFreecell(struct treeNode *current_node,int index,int method)
{
    int addToFoundation = allowAddCardToFoundation(current_node->Freecells[index],current_node->FoundatationRates);

    //move card to Foundation
    if(addToFoundation)
    {
		// Initializing the new child
		struct treeNode *child=(struct treeNode*) malloc(sizeof(struct treeNode));

		if (child==NULL) return -1;


		for(int i=0;i<numberOfFoundations;i++)
        {
            child->FoundatationRates[i] = current_node->FoundatationRates[i];
		}
		for(int i=0; i<numberOfFreecells;i++)
        {
            child->Freecells[i]=current_node->Freecells[i];
        }
        for(int i=0; i<numberOfStacks;i++)
        {
             child->Tableau[i].size = current_node->Tableau[i].size;
            for(int j=0;j<child->Tableau[i].size;j++)
            {
                child->Tableau[i].cards[j]= current_node->Tableau[i].cards[j];
            }

        }

        child->parent=current_node;
		child->g=current_node->g+1;	 	// The depth of the new child


		// add card to Foundation
		child->FoundatationRates[child->Freecells[index].icon]= child->Freecells[index].rate;

        //save the moved cards
		SaveMoveFoundation(child,child->Freecells[index]);

		//remove card from freecell
		child->Freecells[index].icon = -1;
		child->Freecells[index].rate = -1;



		// Check for loops
		if (!checkWithParents(child))
			// In case of loop detection, the child is deleted
			free(child);
		else
		{
		    int error;
            error = addNewNode(child,method);
            if(error<0)
                return -1;

		}
    }

    //check every other stack for this combo
    for(int i=0; i<numberOfStacks;i++)
    {

        struct stack temp;
        temp.size=0;
        push(&temp,current_node->Freecells[index]);
        int addToStack = allowAddCardToCurrentStack(temp,current_node->Tableau[i],1);

        if(addToStack)
        {
            // Initializing the new child
            struct treeNode *child=(struct treeNode*) malloc(sizeof(struct treeNode));

            if (child==NULL) return -1;

            for(int i=0;i<numberOfFoundations;i++)
            {
                child->FoundatationRates[i] = current_node->FoundatationRates[i];
            }
            for(int i=0; i<numberOfFreecells;i++)
            {
                child->Freecells[i]=current_node->Freecells[i];
            }
            for(int i=0; i<numberOfStacks;i++)
            {
                 child->Tableau[i].size = current_node->Tableau[i].size;
                for(int j=0;j<child->Tableau[i].size;j++)
                {
                    child->Tableau[i].cards[j]= current_node->Tableau[i].cards[j];
                }

            }
            child->firstChild =NULL;
            child->parent=current_node;
            child->g=current_node->g+1;	 	// The depth of the new child

            // add card to Tableau
            push(&child->Tableau[i],child->Freecells[index]);

            //save move
            struct stack tempS;
            tempS.size=0;
            push(&tempS,child->Freecells[index]);
            SaveMoveAnotherStack(child,i,tempS);

            //Remove card from freecell
            child->Freecells[index].icon = -1;
            child->Freecells[index].rate = -1;



            // Check for loops
            if (!checkWithParents(child))
                // In case of loop detection, the child is deleted
                free(child);
            else
            {
                int error;
                error = addNewNode(child,method);
                if(error<0)
                    return -1;

            }

        }
    }

    return 1;
}

//This Function return the highest number of cards you can move from this stack
//Input:current stack , the highest combo allowed in this round
int getNumberOfComboCardsInStack(struct stack s,int highestComboAllowed)
{
    if(s.size==0)
        return 0;

    struct card PreviousCard = pop(&s);
    int count=1;
    while(1)
    {
        if(s.size==0 || highestComboAllowed>count)
            break;
        struct card CurrCard = pop(&s);


        if(getColor(CurrCard.icon)!= getColor(PreviousCard.icon) && CurrCard.rate==PreviousCard.rate-1)
        {
            count++;
            PreviousCard = CurrCard;
        }
        else
        {
            break;
        }
    }

    return count;
}

//Get the color of the card
//Input: card's icon
//0--->RED
//1--->BLACK
int getColor(int icon)
{
    if(icon==HEARTS || icon == DIAMONDS)
        return 0;
    if(icon==SPADES || icon == CLUBS)
        return 1;

    return -1;
}


//This function return the highest numbers of cards allowed to move in the current round of game
//The highest combo is equal to the number of free cells + number of empty Tableau stacks + 1
int getHighestComboAllowed(struct stack Tableau[numberOfStacks],struct card Freecells[numberOfFreecells])
{
    int count=1;

    for(int i=0;i<numberOfFreecells;i++)
    {
        if(Freecells[i].rate== -1)
            count++;
    }

    for(int i=0;i<numberOfStacks;i++)
    {
        if(Tableau[i].size == 0)
            count++;

    }

    return count;
}


// A card can added to its Foundation if its rate equals to Foundation last cards rate +1
// 1-->Add card to Foundation
//0 --> cannot add card to Foundation
int allowAddCardToFoundation(struct card c ,int foundationRate[numberOfFoundations])
{
    if(c.rate==-1)
        return 0;
    if(foundationRate[c.icon]+1 == c.rate)
        return 1;

    return 0;
}


// A a combo of cards can added to the target stack if their highest card's rate is equals to target stacks last card's rate -1
// 1-->Add card to Foundation
//0 --> cannot add card to Foundation
int allowAddCardToCurrentStack(struct stack currStack,struct stack stackToAdd,int combo)
{
    int a;
    if(stackToAdd.size==0)
        return 1;

    struct card c1 = getHighestRateCard(&currStack,combo);
    struct card lastCardOfNewStack = getLastCard(&stackToAdd);
    if(c1.rate == lastCardOfNewStack.rate-1 && getColor(c1.icon)!=getColor(lastCardOfNewStack.icon))
    {
        return 1;
    }
    return 0;
}

//Returns the first empty free cell position
//return -1 if all free cells have card
int availableFreecell(struct card c[numberOfFreecells])
{
    for(int i=0; i<numberOfFreecells;i++)
    {
        if(c[i].rate==-1)
            return i; //i freecell empty
    }

    return -1; //all freecells have cards
}


//To win the game you must put all the cards to Foundations
//1->you win!
int gameWon(struct treeNode *current_node)
{
    for (int i=0;i<numberOfFoundations;i++)
    {
        int x = current_node->FoundatationRates[i];
        if(current_node->FoundatationRates[i]<N-1)
            return 0;
    }


    return 1;

}

// Giving a (solution) leaf-node of the search tree, this function computes
// the moves of the blank that have to be done, starting from the root,
// Inputs:
//		struct treeNode *solutionNode	: A leaf-node
// Output:
//		The sequence of  moves that have to be done, starting from the root.
void ExtractSolution(struct treeNode *solution_node,char* filename)
{

    FILE *fout;
	fout=fopen(filename,"w");
	if (fout==NULL)
	{
		printf("Cannot open output file to write solution.\n");
		printf("Now exiting...");
		return;
	}

	struct treeNode *temp_node=solution_node;
	solutionLength=solution_node->g;

	solution= (int*) malloc(solutionLength*sizeof(int));
	temp_node=solution_node;
	//while node != root
	fprintf(fout,"Solution Length: %d \n",solutionLength);

    writeToFile(temp_node,fout,solutionLength);

	fclose(fout);
}

//Write solution to file
void writeToFile(struct treeNode *node,FILE *fout,int size)
{

    char moves[size][50];
    int index = 0;
    while (node->parent!=NULL)
	{


        if(node->cardsMoved.size<1)
        {
            sprintf(moves[index],"Error to this move");
        }
        else if(node->cardsMoved.size==1)
        {
            struct card card= pop(&node->cardsMoved);
            char icon = 'E';
            if(card.icon==0)
                icon='H';
            else if(card.icon==1)
                icon='D';
            else if(card.icon==2)
                icon='S';
            else if(card.icon==3)
                icon='C';

            if(node->moveTo>=0 && node->moveTo<8)
            {
                sprintf(moves[index],"move card %c%d to stack %d\n",icon,card.rate,node->moveTo);
            }
            else if(node->moveTo==8)
                sprintf(moves[index],"move card %c%d to Free cell\n",icon,card.rate);
            else if(node->moveTo==9)
                sprintf(moves[index],"move card %c%d to Foundation\n",icon,card.rate);
            else
                sprintf(moves[index],"Error to this move\n");

        }
        else
        {
            sprintf(moves[index],"move %d cards( ",node->cardsMoved.size);
            while(node->cardsMoved.size>0)
            {
                struct card card = pop(&node->cardsMoved);
                char icon = 'E';
                if(card.icon==0)
                    icon='H';
                else if(card.icon==1)
                    icon='D';
                else if(card.icon==2)
                    icon='S';
                else if(card.icon==3)
                    icon='C';

                sprintf(moves[index]+ strlen(moves[index]),"%c%d ",icon,card.rate);
            }
            sprintf(moves[index]+ strlen(moves[index]),") to stack %d\n",node->moveTo);
        }

        node=node->parent;
        index++;
	}

	for(int k=size; k>-1;k--)
    {
        fprintf(fout,moves[k]);
    }


}


//Display to console the current table of the game
void displayGameTable(struct treeNode *c)
{

	int i,j;
    if(1==1)
    {
    printf("******************************************************************************\n\n");
    printf("--------------------FREECELLS----------------------------------------------- f=%f h=%f g=%d\n",c->f,c->h,(int)c->g);
    for(i=0; i<numberOfFreecells;i++)
    {
        char s = 'E';
                if(c->Freecells[i].icon==0)
                    s='H';
                else if(c->Freecells[i].icon==1)
                    s='D';
                else if(c->Freecells[i].icon==2)
                    s='S';
                else if(c->Freecells[i].icon==3)
                    s='C';
                else
                    continue;


        printf("%c%d ",s,c->Freecells[i].rate);
    }
    printf("\n");

    printf("---------------FOUNDATIONS-----------------------------------------\n");
    for(i=0; i<numberOfFreecells;i++)
    {
        char s = 'E';
                if(i==0)
                    s='H';
                else if(i==1)
                    s='D';
                else if(i==2)
                    s='S';
                else if(i==3)
                    s='C';
                else
                    continue;
        printf("%c%d ",s,c->FoundatationRates[i]);
    }
    printf("\n");
    printf("-------------------------------------------------------------------\n");
        for(i=0;i<numberOfStacks;i++)
        {

            for(j=0;j<c->Tableau[i].size;j++)
            {
                char s = 'E';
                if(c->Tableau[i].cards[j].icon==0)
                    s='H';
                else if(c->Tableau[i].cards[j].icon==1)
                    s='D';
                else if(c->Tableau[i].cards[j].icon==2)
                    s='S';
                else if(c->Tableau[i].cards[j].icon==3)
                    s='C';
                 else
                    continue;
                printf("%c%d ",s,c->Tableau[i].cards[j].rate);
            }

            printf("\n");
        }
        printf("-------------------------------------------------------------------\n");
        printf("-------------------------------------------------------------------\n");
    }

}

int main(int argc, char** argv)
{
    struct stack allStacks[numberOfStacks]={
        {.size = 0 },
        {.size = 0 },
        {.size = 0 },
        {.size = 0 },
        {.size = 0 },
        {.size = 0 },
        {.size = 0 },
        {.size = 0 }
    };
    int method;
	struct treeNode *solutionNode;


    if(argc!=4)
    {
        printf("Wrong number of arguments\n");
        WrongInputMessage();
        return -1;

    }
    method = GetMethod(argv[1]);

	if (method<0)
	{
		printf("Wrong method. Use correct syntax:\n");
		WrongInputMessage();
		return -1;
	}
	int err;
	err=ReadInputFromFile(argv[2], allStacks,numberOfStacks);
	if (err<0)
    {
        printf("Cannot read the input file!\n");
        printf("Press Any Key to exit...\n");
        getch();
        return -1;
    }


    err = verifyInput(allStacks);
    if (err<0)
    {
        printf("Wrong format of the input file!\n");
        printf("Press Any Key to exit...\n");
        getch();
        return -1;
    }

    printf("Solving %s using %s...\n",argv[2],argv[1]);
	t1=clock();

    InitializeSearch(allStacks,method);

	solutionNode=search(method);

	t2=clock();

	if (solutionNode!=NULL)
	{
		printf("Solution found! (%d steps)\n",(int)solutionNode->g);
		printf("Time spent: %f secs\n",((float) t2-t1)/CLOCKS_PER_SEC);
		ExtractSolution(solutionNode,argv[3]);
	}
	else
        printf("No solution found.\n");

    printf("Press Any Key to exit...\n");
    getch();
    return 0;
}
