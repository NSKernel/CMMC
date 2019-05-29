#include <stdlib.h>

#include <ir.h>

extern uint32_t _ir_label_count;

uint32_t _ir_label_last_max = 0;

void ir_compress_label(ir_list *ir_content) {
    int *map;
    int i;
    char changed = 0;
    int local_label_count = _ir_label_count - _ir_label_last_max;
    ir_node *iterator = ir_content->head;
    ir_node *free_node;
    if (local_label_count == 0) {
        // no labels at all
        return;
    }
    map = malloc(sizeof(int) * local_label_count);
    for (i = 0; i < local_label_count; i++) {
        map[i] = _ir_label_last_max + i;
    }
    while (iterator != NULL) {
        if (iterator->content->op == IR_OP_LABEL) {
            if (iterator->next != NULL) {
                if (iterator->next->content->op == IR_OP_LABEL) {
                    // drop next
                    // map next to this
                    changed = 1;
                    map[iterator->next->content->goto_label - _ir_label_last_max] = iterator->content->goto_label;
                    free_node = iterator->next;
                    iterator->next = iterator->next->next;
                    free(free_node->content);
                    free(free_node);
                    continue;
                }
            }
        }
        iterator = iterator->next;
    }
    if (changed) {
        // scan for usage
        iterator = ir_content->head;
        while (iterator != NULL) {
            if (iterator->content->goto_label >= _ir_label_last_max && iterator->content->goto_label < _ir_label_count) {
                iterator->content->goto_label = map[iterator->content->goto_label - _ir_label_last_max];
            }
            iterator = iterator->next;
        }
    }
    _ir_label_last_max = _ir_label_count;
    free(map);
}