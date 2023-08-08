#include <string.h> // For strlen(), strcmp(), strcpy()
#include "markov_chain.h"

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

#define EMPTY -1
#define BOARD_SIZE 100
#define MAX_GENERATION_LENGTH 60
#define RANDOM "Random Walk "
#define USG_ERR "Usage: number of arguments must be 2."

#define DICE_MAX 6
#define NUM_OF_TRANSITIONS 20

/**
 * represents the transitions by ladders and snakes in the game
 * each tuple (x,y) represents a ladder from x to if x<y or a snake otherwise
 */
const int transitions[][2] = {{13, 4},
                              {85, 17},
                              {95, 67},
                              {97, 58},
                              {66, 89},
                              {87, 31},
                              {57, 83},
                              {91, 25},
                              {28, 50},
                              {35, 11},
                              {8,  30},
                              {41, 62},
                              {81, 43},
                              {69, 32},
                              {20, 39},
                              {33, 70},
                              {79, 99},
                              {23, 76},
                              {15, 47},
                              {61, 14}};

/**
 * struct represents a Cell in the game board
 */
typedef struct Cell
{
    int number; // Cell number 1-100
    int ladder_to;  // ladder_to represents the jump of the ladder in case
    // there is one from this square
    int snake_to;  // snake_to represents the jump of the snake in case
    // there is one from this square
    //both ladder_to and snake_to should be -1 if the Cell doesn't have them
} Cell;

typedef struct NeededVals
{
    int seed;
    int length;
} NeededVals;

/** Error handler **/
static int handle_error (char *error_msg, MarkovChain **database)
{
  printf ("%s", error_msg);
  if (database != NULL)
  {
    free_markov_chain (database);
  }
  return EXIT_FAILURE;
}

static int create_board (Cell *cells[BOARD_SIZE])
{
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    cells[i] = malloc (sizeof (Cell));
    if (cells[i] == NULL)
    {
      for (int j = 0; j < i; j++)
      {
        free (cells[j]);
      }
      handle_error (ALLOCATION_ERROR_MASSAGE, NULL);
      return EXIT_FAILURE;
    }
    *(cells[i]) = (Cell) {i + 1, EMPTY, EMPTY};
  }

  for (int i = 0; i < NUM_OF_TRANSITIONS; i++)
  {
    int from = transitions[i][0];
    int to = transitions[i][1];
    if (from < to)
    {
      cells[from - 1]->ladder_to = to;
    }
    else
    {
      cells[from - 1]->snake_to = to;
    }
  }
  return EXIT_SUCCESS;
}

/**
 * fills database
 * @param markov_chain
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int fill_database (MarkovChain *markov_chain)
{
  Cell *cells[BOARD_SIZE];
  if (create_board (cells) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  MarkovNode *from_node = NULL, *to_node = NULL;
  size_t index_to;
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    add_to_database (markov_chain, cells[i]);
  }

  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    from_node = get_node_from_database (markov_chain, cells[i])->data;

    if (cells[i]->snake_to != EMPTY || cells[i]->ladder_to != EMPTY)
    {
      index_to = MAX(cells[i]->snake_to, cells[i]->ladder_to) - 1;
      to_node = get_node_from_database (markov_chain, cells[index_to])
          ->data;
      add_node_to_counter_list (from_node, to_node, markov_chain);
    }
    else
    {
      for (int j = 1; j <= DICE_MAX; j++)
      {
        index_to = ((Cell *) (from_node->data))->number + j - 1;
        if (index_to >= BOARD_SIZE)
        {
          break;
        }
        to_node = get_node_from_database (markov_chain, cells[index_to])
            ->data;
        add_node_to_counter_list (from_node, to_node, markov_chain);
      }
    }
  }
  // free temp arr
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    free (cells[i]);
  }
  return EXIT_SUCCESS;
}

/**
 * copies a cell type and returns the copy
 * @param cell_p cell to be copied
 * @return copied cell
 */
static void *copy_cell (const void *cell_p)
{
  Cell *old_cell = (Cell *) cell_p;
  Cell *cell = malloc (sizeof (Cell));
  if (cell == NULL)
  {
    return NULL;
  }
  cell->ladder_to = old_cell->ladder_to;
  cell->snake_to = old_cell->snake_to;
  cell->number = old_cell->number;
  return cell;
}

/**
 * compares the number in given cells
 * @param a first cell
 * @param b second cell
 * @return the difference in number betweed cell 1 and 2
 */
static int cell_cmp (const void *a, const void *b)
{
  Cell *cell1 = (Cell *) a;
  Cell *cell2 = (Cell *) b;
  return cell1->number - cell2->number;
}

/**
 * prints the data in cell
 * @param cell_p pointer to the cell
 */
static void cell_print (const void *cell_p)
{
  MarkovNode *node = (MarkovNode *) cell_p;
  Cell *cell = node->data;
  if (node->is_last && cell->number != BOARD_SIZE)
  {
    printf ("[%d] -> \n", cell->number);
  }
  else if (cell->snake_to != EMPTY)
  {
    printf ("[%d]-snake to %d -> ", cell->number, cell->snake_to);
  }
  else if (cell->ladder_to != EMPTY)
  {
    printf ("[%d]-ladder to %d -> ", cell->number, cell->ladder_to);
  }
  else if (cell->number == BOARD_SIZE)
  {
    printf ("[%d]\n", cell->number);
  }
  else
  {
    printf ("[%d] -> ", cell->number);
  }
}

/**
 * checks if given cell is last in the list
 * @param cell_p pointer to a cell
 * @return true if last cell, false otherwise
 */
static bool is_last_cell (const void *cell_p)
{
  MarkovNode *node = (MarkovNode *) cell_p;
  Cell *cell = node->data;
  if (cell->number == BOARD_SIZE || node->is_last)
  {
    return true;
  }
  return false;
}

/**
 * frees given cell
 * @param cell_p pointer to cell
 */
static void cell_free (void *cell_p)
{
  MarkovNode *node = (MarkovNode *) cell_p;
  Cell *cell = node->data;
  free (cell);
}

/**
 * fills the chain with functions
 * @param chain markov_chain
 */
static void set_chain (MarkovChain *chain)
{
  chain->copy_func = copy_cell;
  chain->comp_func = cell_cmp;
  chain->print_func = cell_print;
  chain->is_last = is_last_cell;
  chain->free_data = cell_free;
}

/**
 * initiates the linked list
 * @return pointer to initiated linked list
 */
static LinkedList *init_linked_l (void)
{
  LinkedList *linked = malloc (sizeof (*linked));
  if (linked == NULL)
  {
    return NULL;
  }
  linked->first = NULL;
  linked->last = NULL;
  linked->size = 0;
  return linked;
}

/**
 * handles user input
 * @param argc number of args
 * @param argv given args
 * @return seed and sentence number
 */
static NeededVals handle_input (int argc, char *argv[])
{
  if (argc != 3)
  {
    printf (USG_ERR);
    NeededVals ret = {0,0};
    return ret;
  }
  int seed, sent_num;
  sscanf (argv[1], "%d", &seed);
  sscanf (argv[2], "%d", &sent_num);
  NeededVals ret = {seed, sent_num};
  return ret;
}

/**
 * @param argc num of arguments
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main (int argc, char *argv[])
{
  NeededVals need = handle_input (argc, argv);
  int seed = need.seed, sent_num = need.length;
  if (seed == 0 && sent_num == 0)
  {
    return EXIT_FAILURE;
  }
  MarkovChain *chain = malloc (sizeof (MarkovChain));
  if (chain == NULL)
  {
    return EXIT_FAILURE;
  }
  LinkedList *linked = init_linked_l ();
  if (linked == NULL)
  {
    free (chain);
    return EXIT_FAILURE;
  }
  chain->database = linked;
  set_chain (chain);
  int suc = fill_database (chain);
  if (suc == EXIT_FAILURE)
  {
    free (linked);
    free (chain);
    return EXIT_FAILURE;
  }
  srand (seed);
  int i = 1;
  while (sent_num >= i)
  {
    printf (RANDOM);
    printf ("%d: ", i);
    generate_random_sequence (chain, chain->database->first->data,
                              MAX_GENERATION_LENGTH);
    i++;
  }
  free_markov_chain (&chain);
  return EXIT_SUCCESS;
}
