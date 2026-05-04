module security_puf (
    input clk,
    input reset,
    input start,
    input [7:0] challenge,
    output reg [7:0] response,
    output reg done
);
    reg [7:0] lfsr;
    reg [3:0] cycle_count;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            lfsr <= 8'b0;
            done <= 0;
            cycle_count <= 0;
            response <= 8'b0;
        end else if (start && !done) begin
            if (cycle_count == 0) begin
                lfsr <= challenge ^ 8'hA5; // Hardware seed
            end else begin
                // LFSR Polynomial
                lfsr <= {lfsr[6:0], lfsr[7] ^ lfsr[5] ^ lfsr[4] ^ lfsr[3]};
            end

            cycle_count <= cycle_count + 1;

            // Processing delay simulation
            if (cycle_count >= 4) begin
                response <= lfsr;
                done <= 1;
            end
        end else if (!start) begin
            done <= 0;
            cycle_count <= 0;
        end
    end
endmodule