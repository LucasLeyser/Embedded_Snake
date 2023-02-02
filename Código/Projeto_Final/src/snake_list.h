#ifndef _node_h_
#define _node_h_

#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
  int x;
  int y;
  int direction;    /* direction: 0 -> Centro ; 1 -> Direita ; 2 -> Esquerda ; 3 -> Cima ; 4 -> Baixo */
  struct Node* next;
} Node;

Node* snake_create(void);

Node* snake_add (Node* head, int x, int y, int direction);

void snake_free(Node* head);

Node* snake_search(Node* head, int x, int y);

void snake_update(Node* head, int direction);

Node* new_food(Node* head);

int snake_ate(Node* head, Node* food, int direction);

int snake_collision(Node *head);

#endif
