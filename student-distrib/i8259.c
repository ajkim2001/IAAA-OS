/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

// References:  https://wiki.osdev.org/PIC
//              https://www.geeksforgeeks.org/command-words-of-8259-pic/

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* void i8259_init(void)
 * Inputs: none
 * Return Value: none
 * Function: initializes the pic */
void i8259_init(void) {
    master_mask = 0xFF; //disable the PIC
    slave_mask = 0xFF;

    // Use initialization control words to initialize both master and slave PICs
    outb(ICW1, MASTER_8259_PORT_COMMAND);	        // ICW1 uses control bits for Edge and level triggering mode, cascaded mode, call address interval and whether ICW4 is required or not (which it is)
    outb(ICW1, SLAVE_8259_PORT_COMMAND);

    outb(ICW2_MASTER, MASTER_8259_PORT_DATA);	    // ICW2 stores the information regarding the interrupt vector address
    outb(ICW2_SLAVE, SLAVE_8259_PORT_DATA);	 

    outb(ICW3_MASTER, MASTER_8259_PORT_DATA);	    // ICW3 tells master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(ICW3_SLAVE, SLAVE_8259_PORT_DATA);	        // tells slave PIC its cascade identity (0000 0010)

    outb(ICW4, MASTER_8259_PORT_DATA);	            // ICW4 gives additional information about the environment
    outb(ICW4, SLAVE_8259_PORT_DATA);	 

    //restore saved masks
    outb(master_mask, MASTER_8259_PORT_DATA);
    outb(slave_mask, SLAVE_8259_PORT_DATA);  

    enable_irq(ICW3_SLAVE);                         // unmasks the irq that the slave pic is connected to

}

/* void enable_irq(uint32_t irq_num)
 * Inputs: uint32_t irq_num
 * Return Value: none
 * Function: Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    if(irq_num <0 || irq_num >15) {                             // if not a valid irq_num, return
        return; 
    }else if(irq_num <8) {                                      // clear mask for master PIC ports irq_num 0-7
        master_mask = master_mask & ~(1 << irq_num);            // value
        outb(master_mask, MASTER_8259_PORT_DATA);               // outb(value,port_data)
    }else{                                                      // clear mask for slave PIC ports irq_num 8-15
        irq_num-=8;                                             // must shift the irq by 8 to clear the slave PIC ports
        slave_mask = slave_mask & ~(1 << irq_num);
        outb(slave_mask, SLAVE_8259_PORT_DATA); 
    }
}
/* void disable_irq(uint32_t irq_num)
 * Inputs: uint32_t irq_num
 * Return Value: none
 * Function: Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    if(irq_num <0 || irq_num >15){                              // if not a valid irq_num, return
        return;
    }else if(irq_num <8) {                                      // set mask for master PIC ports irq_num 0-7
        master_mask = master_mask | (1 << irq_num);
        outb(master_mask, MASTER_8259_PORT_DATA);
    }else{                                                      // set mask for slave PIC ports irq_num 8-15
        irq_num-=8;                                             // must shift the irq by 8 to set the slave PIC ports
        slave_mask = slave_mask | (1 << irq_num);
        outb(slave_mask, SLAVE_8259_PORT_DATA); 
    }
}

/* void send_eoi(uint32_t irq_num)
 * Inputs: uint32_t irq_num
 * Return Value: none
 * Function: Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    if(irq_num <0 || irq_num >15) {                            // if not a valid irq_num, return
        return;
    }
    /* End-of-interrupt byte.  This gets OR'd with
    * the interrupt number and sent out to the PIC
    * to declare the interrupt finished */
    else if(irq_num <8){                                        // EOI for master PIC ports irq_num 0-7
        outb(EOI | irq_num, MASTER_8259_PORT_COMMAND);          
    }else{                                                      // EOI for slave PIC ports irq_num 8-15
        irq_num-=8;                                            // must shift the irq by 8 to set the slave PIC ports
        outb(EOI | irq_num, SLAVE_8259_PORT_COMMAND);       // EOI to slave port irq_num - 8 AND master port 2 because it is necessary to issue the command to both PIC chips
        outb(EOI | ICW3_SLAVE, MASTER_8259_PORT_COMMAND);
    }
}
