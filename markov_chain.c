#include "linked_list.h"
#include "markov_chain.h"
#include <string.h>
#include <stdio.h>  // For printf(), sscanf()
#include <stdlib.h> // For exit(), malloc()
#include <stdbool.h> // for bool

/**
* Get random number between 0 and max_number [0, max_number).
* @param max_number maximal number to return (not including)
* @return Random number
*/
int get_random_number (int max_number)
{
  return rand () % max_number;
}

Node *add_to_database (MarkovChain *markov_chain, void *data_ptr)
{
  Node *cur = get_node_from_database (markov_chain, data_ptr);
  if (cur != NULL)
  {
    return cur;
  }
  MarkovNode *markov_node = malloc (sizeof (*markov_node));
  if (markov_node == NULL)
  {
    printf (ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }
  void *temp = markov_chain->copy_func (data_ptr);
  if (temp == NULL)
  {
    free (markov_node);
    printf (ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }
  markov_node->data = temp;
  markov_node->next_node_ctr = 0;
  markov_node->counter_list = NULL;
  markov_node->is_last = false;
  int suc = add (markov_chain->database, markov_node);
  if (suc == 1)
  {
    free (markov_node->data);
    free (markov_node);
    printf (ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }
  return markov_chain->database->last;
}

Node *get_node_from_database (MarkovChain *markov_chain, void *data_ptr)
{
  Node *cur = markov_chain->database->first;
  for (int i = 0; i < markov_chain->database->size; i++)
  {
    if (markov_chain->comp_func (cur->data->data, data_ptr) == 0)
    {
      return cur;
    }
    cur = cur->next;
  }
  return NULL;
}

NextNodeCounter *check_ctr_list (MarkovChain *markov_chain, MarkovNode *node,
                                 void *data)
{
  int size = node->next_node_ctr;
  for (int i = 0; i < size; i++)
  {
    if (markov_chain->comp_func (node->counter_list[i].markov_node->data, data)
        == 0)
    {
      return &(node->counter_list[i]);
    }
  }
  return NULL;
}

bool new_node_handle (MarkovNode *first_node, MarkovNode *second_node)
{
  first_node->counter_list = malloc (sizeof (NextNodeCounter));
  if (first_node->counter_list == NULL)
  {
    printf (ALLOCATION_ERROR_MASSAGE);
    return false;
  }
  first_node->counter_list[0].markov_node = second_node;
  first_node->counter_list[0].frequency = 1;
  first_node->next_node_ctr = 1;
  return true;
}

bool extend_node (MarkovChain *markov_chain, MarkovNode *first_node,
                  MarkovNode *second_node)
{
  NextNodeCounter *temp = check_ctr_list (markov_chain, first_node,
                                          second_node->data);
  if (temp == NULL)
  {
    NextNodeCounter *check = realloc (first_node->counter_list, \
    sizeof (NextNodeCounter) * (first_node->next_node_ctr + 1));
    if (check == NULL)
    {
      printf (ALLOCATION_ERROR_MASSAGE);
      return false;
    }
    first_node->counter_list = check;
    first_node->next_node_ctr++;
    first_node->counter_list[first_node->next_node_ctr - 1].markov_node =
        second_node;
    first_node->counter_list[first_node->next_node_ctr - 1].frequency = 1;
  }
  else
  {
    temp->frequency++;
  }
  return true;
}

bool add_node_to_counter_list (MarkovNode *first_node, MarkovNode
*second_node, MarkovChain *markov_chain)
{
  if (first_node->counter_list == NULL)
  {
    return new_node_handle (first_node, second_node);
  }
  else
  {
    return extend_node (markov_chain, first_node, second_node);
  }
}

void free_markov_chain (MarkovChain **ptr_chain)
{
  MarkovChain chain = **ptr_chain;
  Node *cur = chain.database->first;
  Node *temp = NULL;
  while (cur != NULL)
  {
    temp = cur->next;
    free (cur->data->counter_list);
    chain.free_data (cur->data);
    free (cur->data);
    free (cur);
    cur = temp;
  }
  free (chain.database);
  free (*ptr_chain);
}

MarkovNode *get_first_random_node (MarkovChain *markov_chain)
{
  Node *cur = NULL;
  do
  {
    int i = get_random_number (markov_chain->database->size);
    cur = markov_chain->database->first;
    for (int j = 0; j < i; j++)
    {
      cur = cur->next;
    }
  }
  while (markov_chain->is_last (cur->data) == true);
  return cur->data;
}

int get_total_nodes (MarkovNode *state_struct_ptr)
{
  int ret = 0;
  int range = state_struct_ptr->next_node_ctr;
  for (int i = 0; i < range; i++)
  {
    int tmp = state_struct_ptr->counter_list[i].frequency;
    ret += tmp;
  }
  return ret;
}

MarkovNode *get_next_random_node (MarkovNode *state_struct_ptr)
{
  if (state_struct_ptr->counter_list == NULL)
  {
    return NULL;
  }
  int nodes_num = get_total_nodes (state_struct_ptr);
  int i = get_random_number (nodes_num);
  int j = 0;
  MarkovNode *cur = NULL;
  while (i >= 0)
  {
    cur = state_struct_ptr->counter_list[j].markov_node;
    i -= state_struct_ptr->counter_list[j].frequency;
    j++;
  };
  return cur;
}

void generate_random_sequence (MarkovChain *markov_chain, MarkovNode *
first_node, int max_length)
{
  MarkovNode *cur = NULL;
  if (max_length < 2)
  {
    return;
  }
  if (first_node == NULL)
  {
    cur = get_first_random_node (markov_chain);
  }
  else
  {
    cur = first_node;
  }
  markov_chain->print_func (cur);
  int cur_len = 1;
  while (cur_len < max_length)
  {
    cur = get_next_random_node (cur);
    if (cur == NULL)
    {
      return;
    }
    if (cur_len == max_length - 1)
    {
      cur->is_last = true;
    }
    if (markov_chain->is_last (cur))
    {
      markov_chain->print_func (cur);
      cur->is_last = false;
      return;
    }
    markov_chain->print_func (cur);
    cur_len++;
  }
}