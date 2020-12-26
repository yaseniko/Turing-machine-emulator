/// README:
// Данная программа эмулирует работу машины Тьюринга (МТ)
// Запуск программы осуществляется так: <название исполняемого файла> <название файла, где находится программа для эмулируемой МТ> <вход эмулируемой МТ> <мод (d - debug, r - release)>
// Например: ./a.out source.txt input.txt r
// В release моде программа сразу выдаст выхлоп МТ. В debug моде можно нажимать 'n' для перехода к следующему состоянию или 'c' для выполнения программы до конца
// Программа для МТ состоит из строчек вида <нынешнее состояние> <нынешний символ> <новый символ> <куда двигаться (r - вправо, l - влево, * - никуда)> <новое состояние>
// Машина останавливается как только достигает состояния halt (и только его)


#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define FIX_UNUSED(X) (void (X))

enum class debug_command
{
  next_step,
  until_end,
  non_debug,

  COUNT
};

class bi_list
{
private:
  char value = '_';

public:
  bi_list *left = nullptr;
  bi_list *right = nullptr;

public:
  char get_value () { return value; }

  void set_value (char v)
  {
    if (v == '*')
      return;
    else
      value = v;
  }
};


class state
{
public:
  char name[256];
  char next_state[256];
  char in_symbol = '_';
  char out_symbol = '_';
  char move = '*';

public:
  state () = default;

  state (std::string name_arg, char in_symbol_arg)
  {
    strcpy (name, name_arg.c_str ());
    in_symbol = in_symbol_arg;
  }

  void print ()
  {
    printf ("Last executed state: %s %c %c %c %s\n", name, in_symbol, out_symbol, move, next_state);
  }
};


class TM
{
private:
  bi_list *curr = nullptr;
  std::vector<state> states;

public:
  void clean ();
  debug_command scan_debug_command ();
  bool fill_states (FILE *source);
  bool init_tape_by_input (FILE *input);
  bool move_carriage (char direction);
  void move_carriage_to_beginning ();
  void print_answer ();
  bool run_simulation (char mode);
};


void TM::clean ()
{
  move_carriage_to_beginning ();

  while (curr->right)
    {
      curr = curr->right;
      delete curr->left;
    }

  delete curr;
}


bool TM::fill_states (FILE *source)
{
  state tmp;

  while (fscanf (source, "%s %c %c %c %s", tmp.name, &tmp.in_symbol, &tmp.out_symbol, &tmp.move, tmp.next_state) == 5)
    {
      if (tmp.move != 'r' && tmp.move != 'l' && tmp.move != '*')
        {
          printf ("Error! Moving symbols are only 'l', 'r' and '*'\n");
          return false;
        }

      states.push_back (tmp);
    }

  auto comparator = [](auto &st_1, auto &st_2)
  {
    return (strcmp (st_1.name, st_2.name) < 0) || (strcmp (st_1.name, st_2.name) == 0 && st_2.in_symbol == '*');
  };

  std::sort (states.begin (), states.end (), comparator);
  return true;
}


bool TM::init_tape_by_input (FILE *input)
{
  char in;
  fscanf (input, "%c", &in);
  curr = new bi_list ();
  curr->set_value (in);

  while (fscanf (input, "%c", &in) == 1)
    {
      if (in == '\n')
        continue;

      curr->right = new bi_list ();
      if (!curr->right)
        return false;

      curr->right->set_value (in);
      curr->right->left = curr;
      curr = curr->right;
    }

  move_carriage_to_beginning ();
  return true;
}


bool TM::move_carriage (char direction)
{
  if (direction == 'r')
    {
      if (!curr->right)
        {
          curr->right = new bi_list ();
          if (!curr->right)
            return false;

          curr->right->left = curr;
        }

      curr = curr->right;
    }

  else if (direction == 'l')
    {
      if (!curr->left)
        {
          curr->left = new bi_list ();
          if (!curr->left)
            return false;

          curr->left->right = curr;
        }

      curr = curr->left;
    }

  return true;
}


void TM::move_carriage_to_beginning ()
{
  while (curr->left)
    curr = curr->left;
}


void TM::print_answer ()
{
  bi_list *tmp = curr;
  while (tmp->left)
    tmp = tmp->left;

  while (tmp)
    {
      if (tmp->get_value () != '_')
        printf ("%c", tmp->get_value ());

      tmp = tmp->right;
    }

  printf ("\n");
}


debug_command TM::scan_debug_command ()
{
  char command;
  scanf ("%c", &command);

  if (command == 'n')
    return debug_command::next_step;
  else if (command == 'c')
    return debug_command::until_end;
  else
    {
      if (command != '\n')
        printf ("Please enter 'n' (next step) or 'c' (go to the end)\n");
      return scan_debug_command ();
    }
}


bool TM::run_simulation (char mode)
{
  debug_command command = debug_command::non_debug;

  if (mode == 'd')
    {
      printf ("Hello in debug mode!\nPress 'n' to go to the next step\n");
      printf ("Press 'c' to run the programm until the end\n\n");

      print_answer ();
      command = scan_debug_command ();
    }

  state curr_state ("0", curr->get_value ());
  while (strcmp (curr_state.name, "halt") != 0)
    {
      auto iter = std::find_if (states.begin (), states.end (),
                                [&](auto &state) { return (strcmp (curr_state.name, state.name) == 0) && (curr_state.in_symbol == state.in_symbol || state.in_symbol == '*'); });

      if (iter == states.end ())
        {
          printf ("Error! There is no state %s with symbol %c\n", curr_state.name, curr_state.in_symbol);
          return -1;
        }

      curr->set_value (iter->out_symbol);
      if (!move_carriage (iter->move))
        {
          printf ("Error! Allocation failure! Most likely the Turing machine went into an infinite loop!\n");
          return -1;
        }

      curr_state = state ((strcmp (iter->next_state, "*") == 0) ? curr_state.name : iter->next_state, curr->get_value ());

      if (command == debug_command::next_step)
        {
          print_answer ();
          iter->print ();
          command = scan_debug_command ();
        }
    }

  print_answer ();
  return 0;
}


int main (int argc, char **argv)
{
  if (argc != 4 || (*argv[3] != 'd' && *argv[3] != 'r'))
    {
      printf ("Usage: %s <file with machine code> <file with input> <mode(r/d)>\n", argv[0]);
      return -1;
    }

  FILE *source = fopen (argv[1], "r");
  FILE *input = fopen (argv[2], "r");

  if (!source || !input)
    {
      printf ("Invalid input file(s)\n");
      return -1;
    }

  TM machine;
  if (!machine.fill_states (source))
    return -1;

  if (!machine.init_tape_by_input (input))
    return -1;

  machine.run_simulation (*argv[3]);
  machine.clean ();

  fclose (source);
  fclose (input);

  return 0;
}
