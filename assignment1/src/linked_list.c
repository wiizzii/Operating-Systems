/*
 * linked_list.c
 *
 *  Created on: Dec 24, 2013
 *      Author: jacob
 */
#include "linked_list.h"
#include <stdlib.h>
#include <stdio.h>

linked_list *init_linked_list() {
  linked_list *head = malloc(sizeof(struct linked_list));
	head->data = NULL;
	head->next = NULL;
  head->previous = NULL;
	return head;
}

void add_element( linked_list *list, void *element) {
	linked_list *current = list;
	while (current->next != NULL) {
		current = current->next;
	}
	current -> next = malloc(sizeof(struct linked_list));
	current -> next -> data = element;
	current -> next -> next = NULL;
	current -> next -> previous = current;
}

int linked_list_size(linked_list *list) {
	linked_list *current = list;
	if (current -> next == NULL) {
		return 0;
	} else{
		int i = 0;
		while (current->next != NULL){
			current = current ->next;
			i++;
		}
		return i;
	}
}

void *remove_first(linked_list *list) {
    if (list->next == NULL){
        return NULL;
    }else{
        void *rm_element = list->next->data;
        if (list->next->next == NULL) {
            free(list->next);
            return rm_element;
        }else{
            linked_list *bla= list->next->next;
            free(list->next);
            list->next =bla;
            list->next->previous = list;
            return rm_element;
        }
    }
}

int remove_element(linked_list *list, void *element) {
	linked_list *current = list->next;
	while (current != NULL){
		if (current-> data == element){
			if (current->next !=NULL){
				current->previous ->next = current->next;
				current->next->previous = current->previous;
			}else{
				current->previous->next = NULL;
			}
			free(current);
			return 0;
		}else{
			current = current->next;

		}
	}
	return -1;
}
int main() {
	linked_list *mylist = init_linked_list();
	add_element(mylist, (void*)'1');
	add_element(mylist, "2");
	add_element(mylist, "3");
	int l = linked_list_size(mylist);
	printf("first %d \n",l);
	remove_first(mylist);
	l = linked_list_size(mylist);
	printf("sec %d \n",l);
	remove_element(mylist, (void*)'1');
	l = linked_list_size(mylist);
	printf("thrd %d \n",l);
}
