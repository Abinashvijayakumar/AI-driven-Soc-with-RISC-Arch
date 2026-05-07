#include <iostream>
#include <string>
#include "Vsecurity_puf.h"
#include "verilated.h"
#include "verilated_vcd_c.h" // ADDED FOR GTKWAVE

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vsecurity_puf* top = new Vsecurity_puf;

    // Initialize Tracing
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("waveform.vcd"); // Creates the file for GTKWave

    int main_time = 0; // Tracks absolute simulation time
    
    top->clk = 0; top->reset = 1; top->start = 0; top->challenge = 0;
    
    top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
    top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
    top->reset = 0;

    std::string input_str;
    while (std::cin >> input_str) {
        if (input_str == "EXIT") break;
        int challenge_val = std::stoi(input_str);
        
        top->challenge = challenge_val; top->start = 1;
        
        int tick_count = 0;
        while (!top->done && tick_count < 100) {
            top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
            top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
            tick_count++;
        }
        
        std::cout << "{\"challenge\":" << challenge_val 
                  << ",\"response\":" << (int)top->response 
                  << ",\"ticks\":" << tick_count << "}" << std::endl;
                  
        top->start = 0; 
        top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
        top->clk = !top->clk; top->eval(); tfp->dump(main_time++);
        
        // Safety switch: Stop recording waveform after 2000 cycles to prevent gigabyte-sized files
        if (main_time > 2000) {
            tfp->close();
        }
    }

    top->final();
    tfp->close();
    delete top;
    return 0;
}