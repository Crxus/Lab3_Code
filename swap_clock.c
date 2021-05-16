#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clock.h>
#include <list.h>



list_entry_t pra_list_head;   //the recording list, a pointer points to the oldest page

static int
_clock_init_mm(struct mm_struct *mm)
{     
     list_init(&pra_list_head);
     mm->sm_priv = &pra_list_head;
     //here's the using of sm_priv--to point to the list that decide the swapping of pages
     //cprintf(" mm->sm_priv %x in fifo_init_mm\n",mm->sm_priv);
     return 0;
}

/*
 * (3)_clock_map_swappable: insert the new page to the end of the list, the pointer points to the beginning of the list
 */

static int
_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;   //we have to get the list of accessing records
    //ATTENTION: sm_priv is a void pointer, that means it could be transferred to any other types
    list_entry_t *entry=&(page->pra_page_link);   //get the page's link in pra
 
    assert(entry != NULL && head != NULL);    //if null, the pra or the page is not exist
    //record the page access situlation
    list_add_before(head,entry);    //Add the new page to the tail of the list
    return 0;  // the swappable function should return a 0 to send a message that this fucnction has done its job correctly.
}


/*
 *   #define PTE_A           0x020                   // Accessed      6th bit
#define PTE_D           0x040                   // Dirty         7th bit
 *  (4)_clock_swap_out_victim: According CLOCK PRA, if PTE_A==1, turn it to 0, if PTE_A==0, choose the page whose PTE_D==0, if there is no
 *   page whose PTE_D==0, choose randomly. 
 */
static int
_clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
     list_entry_t *head=(list_entry_t*)mm->sm_priv;
         assert(head != NULL);
     assert(in_tick==0);  //we are not about to use it in the FIFO
     /* Select the victim */
     list_entry_t *le= head->next;  //now we guess the head of pra does not save anything, its first not null index is head->next
     //assert(le!=NULL);
     assert(le!=head);
     int turn=0;
     while(1)
     {
        if(le==head)
        {
            turn++;
            le=le->next;
        }
        struct Page *page=le2page(le,pra_page_link);
        assert(page!=NULL);
        if(turn==2)//means every page's dirty bit is 1, then we choose a page randomly
        {
            *ptr_page=page;
            list_del(le);  //delete the page from the pra list
            break;
        }
        pte_t *pte=get_pte(mm->pgdir,page->pra_vaddr,0); //if pte not exist, do not create a new pte.
        if((*pte&PTE_A)==0)//if access bit is 0, we check the dirty bit, if dirty bit is also 0, we can swap it out.
        {
            if((*pte&PTE_D)==0)
            {
                *ptr_page=page;
                list_del(le);
                break;
            }
        }
        else//access bit is 1, we change it to 0.
        {
            *pte &= ~PTE_A;
        }
        le=le->next;
     }
     return 0;
}

static int
_clock_check_swap(void) {
    cprintf("write Virt Page c in clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==4);
    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==4);
    cprintf("write Virt Page d in clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==4);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==4);
    cprintf("write Virt Page e in clock_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==5);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==5);
    cprintf("write Virt Page a in clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==6);
    cprintf("write Virt Page b in clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==7);
    cprintf("write Virt Page c in clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==8);
    cprintf("write Virt Page d in clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==9);
    cprintf("write Virt Page e in clock_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==10);
    cprintf("write Virt Page a in clock_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==11);
    return 0;
}


static int
_clock_init(void)
{
    return 0;
}

static int
_clock_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_clock_tick_event(struct mm_struct *mm)
{ return 0; }


struct swap_manager swap_manager_clock =
{
     .name            = "clock swap manager",
     .init            = &_clock_init,
     .init_mm         = &_clock_init_mm,
     .tick_event      = &_clock_tick_event,
     .map_swappable   = &_clock_map_swappable,
     .set_unswappable = &_clock_set_unswappable,
     .swap_out_victim = &_clock_swap_out_victim,
     .check_swap      = &_clock_check_swap,
};