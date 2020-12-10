// path for a huffman coder
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "huff.h"

// create a new huffcoder structure
struct huffcoder *huffcoder_new()
{
  struct huffcoder *result = malloc(sizeof(struct huffcoder));
  return result;
}

// count the frequency of characters in a file; set chars with zero
// frequency to one
void huffcoder_count(struct huffcoder *this, char *filename)
{
  FILE *file = fopen(filename, "r");
  int temp = 0;
  while (temp != EOF)
  {
    temp = fgetc(file);
    if (temp != EOF)
    {
      this->freqs[temp]++;
    }
  }
  for (int i = 0; i < NUM_CHARS; i++)
  {
    if (this->freqs[i] == 0)
    {
      this->freqs[i] = 1;
    }
  }
  fclose(file);
}

struct huffchar *huffchar_new(unsigned char c, int freq)
{
  struct huffchar *result = malloc(sizeof(struct huffchar));
  result->u.compound.left = 0;
  result->u.compound.right = 0;
  result->is_compound = 0;
  result->freq = freq;
  result->seqno = c;
  result->u.c = c;
  return result;
}

int getSmallestIndex(struct huffchar *tree[NUM_CHARS])
{
  unsigned int least_freq = UINT_MAX;
  int seq_no = 0;
  int index = -1;
  for (int i = 0; i < NUM_CHARS; i++)
  {
    if (tree[i] != NULL)
    {
      int current_freq = tree[i]->freq;
      int current_seqno = tree[i]->seqno;
      if (current_freq < least_freq)
      {
        index = i;
        seq_no = current_seqno;
        least_freq = current_freq;
      }
      else if (current_freq == least_freq && current_seqno < seq_no)
      {
        index = i;
        seq_no = current_seqno;
        least_freq = current_freq;
      }
    }
  }
  return index;
}

struct huffchar *make_compound(struct huffchar *n1, struct huffchar *n2, int seq_no)
{
  struct huffchar *compound = malloc(sizeof(struct huffchar));
  compound->freq = n1->freq + n2->freq;                        
  compound->is_compound = 1;                                   
  compound->u.compound.left = n1;                              
  compound->u.compound.right = n2;                             
  compound->seqno = seq_no;
  return compound;
}

//check for non null nodes
int treeSize(struct huffchar *tree[NUM_CHARS])
{
  int size = 0;
  for (int i = 0; i < NUM_CHARS; i++)
  {
    if (tree[i] != NULL)
    {
      size++;
    }
  }
  return size;
}
// using the character frequencies build the tree of compound
// and simple characters that are used to compute the Huffman codes
void huffcoder_build_tree(struct huffcoder *this)
{
  struct huffchar *tree[NUM_CHARS];
  for (int i = 0; i < NUM_CHARS; i++)
  {
    tree[i] = huffchar_new((unsigned char)i, this->freqs[i]);
  }

  int current_seq_no = NUM_CHARS;
  int index1 = 0;
  int index2 = 0;

  while (treeSize(tree) > 1)
  {
    //find lowest 2 and take them out of list
    index1 = getSmallestIndex(tree);
    struct huffchar *least = tree[index1];
    tree[index1] = NULL;

    index2 = getSmallestIndex(tree);
    struct huffchar *secondLeast = tree[index2];
    tree[index2] = NULL;

    struct huffchar *compound = make_compound(least, secondLeast, current_seq_no);
    current_seq_no++;
    tree[index1] = compound;
  }
  index1 = getSmallestIndex(tree);
  this->tree = tree[index1];
}

char *int_to_binary_str(int path, int code_length)
{
  int digit;
  char *binary_str;

  int temp = 0;
  binary_str = (char *)malloc(NUM_CHARS + 1);

  if (binary_str == NULL)
    exit(EXIT_FAILURE);

  for (int c = code_length - 1; c >= 0; c--)
  {
    digit = path >> c;

    if (digit & 1)
      *(binary_str + temp) = 1 + '0';
    else
      *(binary_str + temp) = 0 + '0';

    temp++;
  }
  *(binary_str + temp) = '\0';

  return binary_str;
}

void tree2table_recursive(struct huffcoder *this, struct huffchar *node, int path, int depth)
{
  if (node->is_compound)
  {
    path = path << 1;
    tree2table_recursive(this, node->u.compound.left, path, depth + 1);
    path = path | 1;
    tree2table_recursive(this, node->u.compound.right, path, depth + 1);
  }
  if (!node->is_compound)
  {
    unsigned char index = node->u.c;
    this->code_lengths[(unsigned int)index] = depth;

    this->codes[(unsigned int)index] = int_to_binary_str(path, depth);
  }
}

// using the Huffman tree, build a table of the Huffman codes
// with the huffcoder object
void huffcoder_tree2table(struct huffcoder *this)
{
  int path = 0;
  int depth = 0;
  tree2table_recursive(this, this->tree, path, depth);
}

// print the Huffman codes for each character in order;
// you should not modify this function
void huffcoder_print_codes(struct huffcoder *this)
{
  for (int i = 0; i < NUM_CHARS; i++)
  {
    // print the code
    printf("char: %d, freq: %d, code: %s\n", i, this->freqs[i], this->codes[i]);
    ;
  }
}

// encode the input file and write the encoding to the output file
void huffcoder_encode(struct huffcoder *this, char *input_filename, char *output_filename)
{
  FILE *inputFile = fopen(input_filename, "r");
  FILE *outputFile = fopen(output_filename, "w");

  int c = fgetc(inputFile);
  while (c != EOF)
  {
    char *codeOfC = this->codes[c];
    fputs(codeOfC, outputFile);
    c = fgetc(inputFile);
  }
  fputs(this->codes[4], outputFile);
  fclose(inputFile);
  fclose(outputFile);
}

// decode the input file and write the decoding to the output file
void huffcoder_decode(struct huffcoder *this, char *input_filename, char *output_filename)
{
  FILE *inputFile = fopen(input_filename, "r");
  FILE *outputFile = fopen(output_filename, "w");

  unsigned char outputChar;
  unsigned char EOFchar = 4;
  unsigned char inputChar = fgetc(inputFile);
  while (outputChar != EOFchar)
  {
    struct huffchar *ptr = this->tree;
    while (ptr->is_compound)
    {
      if (inputChar == '0')       { ptr = ptr->u.compound.left; }
      else  if (inputChar == '1') { ptr = ptr->u.compound.right; }
      inputChar = fgetc(inputFile);
    }
    outputChar = ptr->u.c;
    if (outputChar != 4) { fputc(outputChar, outputFile); }
  }
}
