#include <iostream>
#include <string>
#include "Vsecurity_puf.h"
#include "verilated.h"

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vsecurity_puf* top = new Vsecurity_puf;

    top->clk = 0;
    top->reset = 1;
    top->start = 0;
    top->challenge = 0;
    
    top->clk = !top->clk; top->eval();
    top->clk = !top->clk; top->eval();
    top->reset = 0;

    std::string input_str;
    while (std::cin >> input_str) {
        if (input_str == "EXIT") break;
        int challenge_val = std::stoi(input_str);
        
        top->challenge = challenge_val;
        top->start = 1;
        
        int tick_count = 0;
        while (!top->done && tick_count < 100) {
            top->clk = !top->clk; top->eval();
            top->clk = !top->clk; top->eval();
            tick_count++;
        }
        
        std::cout << "{\"challenge\":" << challenge_val 
                  << ",\"response\":" << (int)top->response 
                  << ",\"ticks\":" << tick_count << "}" << std::endl;
                  
        top->start = 0; 
        
        // [PM Lucci Patch] Force the hardware to recognize the start pin going low
        top->clk = !top->clk; top->eval();
        top->clk = !top->clk; top->eval();
    }

    delete top;
    return 0;
}