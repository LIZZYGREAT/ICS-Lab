#include "nemu.h"
#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <assert.h>
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp() {
  if (free_ == NULL) {
    printf("Error: No free watchpoints available. Maximum limits reached.\n");
    assert(0);
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  return wp;
}

void free_wp(WP *wp) {
  if (wp == NULL || head == NULL) {
    return;
  }

  if (head == wp) {
    head = head->next;
  } else {
    WP *curr = head;
    while (curr != NULL && curr->next != wp) {
      curr = curr->next;
    }
    
    if (curr != NULL) {
      curr->next = wp->next;
    } else {
      printf("Error: Watchpoint %d is not in the active list.\n", wp->NO);
      assert(0);
    }
  }

  wp->old_val = 0;
  memset(wp->expr, 0, sizeof(wp->expr));
  wp->next = free_;
  free_ = wp;
}
bool delete_wp_by_no(int no) {
  WP *curr = head;
  while (curr != NULL) {
    if (curr->NO == no) {
      free_wp(curr);
      return true;
    }
    curr = curr->next;
  }
  return false; 
}

void print_wp() {
  if (head == NULL) {
    printf("No watchpoints currently set.\n");
    return;
  }

  printf("%-4s\t%-12s\t%s\n", "NO", "Old Value", "Expression");
  printf("--------------------------------------------------\n");

  WP *curr = head;
  while (curr != NULL) {
    printf("%-4d\t0x%08x\t%s\n", curr->NO, curr->old_val, curr->expr);
    curr = curr->next;
  }
}
