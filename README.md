# Battery_management_system
The usage of electric vehicles is growing day by day in India. The battery is one of the essential 
components of the electric vehicle. Battery management system (BMS) performance needs to be 
improved in order to make batteries a safe, reliable, and cost-efficient solution. The BMS should 
have precise algorithms for measuring and estimating the battery's functioning status and be 
designed with the finest protections to protect the battery from hazardous and ineffective use. The 
majority of electric cars now use lithium-ion batteries. To optimize the lithium-ion cells' life, we 
must ensure that we bring a cell balance to equalize the overall battery of each cell in the pack. Cell 
balancing is the technique of equalizing the voltages among the cells to maximize the battery pack's 
efficiency. Passive cell balancing drains charge from cells having too much charge and dissipates 
drained energy as heat through a resistor.
This project focuses on developing a passive cell balancing system for a 15-cell lithium-ion battery 
pack to ensure that each cell is charged and discharged equally. The project involved monitoring 
each cell voltage and balancing them using a bQ76PL455EVM voltage monitoring board and 
TMS570LS04 controller board. Halcogen and Code Composer Studio were used as development 
environments. Halcogen was used to configure the microcontroller's interrupts, peripherals, clocks, 
and other parameters. Initialization codes for peripherals and drivers were developed and included 
in Code Composer Studio (CCS). Algorithm was developed using CCS to determine each cell 
voltage, minimum cell voltage and balance each cell. Cell balancing was also done by using 
bq76PL455A-Q1 Graphical User Interface (GUI). 
