#include "predicate.h"
#include <stdio.h>

int main(int argc, char** argv) {
  init_pred();

  printf("set predicate...\n");
  set_pred_true();

// predicate:
//   printf("evaluate predicate: ");
  if (get_pred()) {
    printf("TRUE\n");
  }
  else {
    printf("FALSE\n");

    // printf("end of program!\n");
    // return 0;
  }

  // printf("continue and reset predicate...\n");
  // set_pred_false();
  // goto predicate;
}
