`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/30 12:09:29
// Design Name: 
// Module Name: HDMI_OUT
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module HDMI_OUT(
    input wire          s_rst_n,
    
    input wire [23:0]   hdmi_data,
    input wire          hdmi_pclk,
    input wire          hdmi_hs,
    input wire          hdmi_vs,
    input wire          hdmi_de,
    input wire          hdmi_vs_sel,
    
    input wire [15:0]   result_x,
    input wire [15:0]   result_y,
    input wire [15:0]   result_w,
    input wire [15:0]   result_h,
    
    output wire [23:0] hdmi_out_data,
    output wire        hdmi_out_hs,
    output wire        hdmi_out_vs,
    output wire        hdmi_out_de

    );
parameter IMG_width = 1280;
parameter IMG_height = 720;
reg hdmi_vs_sel_r = 'd0;
always @(posedge hdmi_pclk or negedge s_rst_n)begin
	if(!s_rst_n) begin	
		hdmi_vs_sel_r <= 'd0;
    end
    else begin
        hdmi_vs_sel_r <= hdmi_vs_sel;
    end
end 
reg				hdmi_in_vs_posedge_r1	= 'd0		;
reg				hdmi_in_vs_posedge_r2	= 'd0		;
reg				hdmi_in_vs_posedge_r	= 'd0		;
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_in_vs_posedge_r1 <= 'd0;
			hdmi_in_vs_posedge_r2 <= 'd0;
		end
		else begin
			hdmi_in_vs_posedge_r1 <= hdmi_vs;
			hdmi_in_vs_posedge_r2 <= hdmi_in_vs_posedge_r1;
		end
	end

	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_in_vs_posedge_r <= 'd0;
		end
		else begin
			if((hdmi_in_vs_posedge_r1=='d1) &&(hdmi_in_vs_posedge_r2=='d0)) begin
				hdmi_in_vs_posedge_r <= 'd1;
			end
			else begin
				hdmi_in_vs_posedge_r <= 'd0;
			end
		end
	end
	
reg [15:0] x_r = 'd0;
reg [15:0] y_r = 'd0;
reg [15:0] w_r = 'd0;
reg [15:0] h_r = 'd0; 
reg [11:0]x_pixel='d0;
reg [11:0]y_pixel='d0;
always @(posedge hdmi_pclk or negedge s_rst_n)begin
	if(!s_rst_n) begin	
        x_r <= 'd0;
        y_r <= 'd0;
        w_r <= 'd0;
        h_r <= 'd0;
    end
    else if(hdmi_vs_sel_r)begin//跳帧有效时，再延迟一个周期赋值
            x_r <= result_x;
            y_r <= result_y;
            w_r <= result_w;
            h_r <= result_h;
    end
end
always@(posedge hdmi_pclk or negedge s_rst_n)begin
    if(!s_rst_n)begin
        x_pixel <= 'd0;
    end
    else if(hdmi_de)begin
        if(x_pixel < IMG_width )begin
             x_pixel <= x_pixel + 1'b1;  
        end
        else begin
             x_pixel <= 'd1; 
        end
    end
    else if(hdmi_in_vs_posedge_r)begin
            x_pixel <= 'd0;
    end
    else begin
        x_pixel <= 'd0;
    end
end

always@(posedge hdmi_pclk or negedge s_rst_n)begin
    if(!s_rst_n)begin
        y_pixel <= 'd0;
    end
    else if(x_pixel == IMG_width)begin
            y_pixel <= y_pixel + 1'b1;
        end
    else if(y_pixel == IMG_height)begin
            y_pixel <= 'd0; 
        end
    else if(hdmi_in_vs_posedge_r)begin
            y_pixel <= 'd0;
    end
end
	reg		[23:0]	hdmi_in_data_r			='d0		;
	reg 			hdmi_in_hs_r			='d0		;
	reg 			hdmi_in_vs_r			='d0		;
	reg 			hdmi_in_de_r			='d0		;
	
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin	
			hdmi_in_data_r	<= 'd0;
			hdmi_in_hs_r	<= 'd0;
			hdmi_in_vs_r	<= 'd0;
			hdmi_in_de_r	<= 'd0;
		end
		else begin
				hdmi_in_data_r	<=  hdmi_data	;
				hdmi_in_hs_r	<=  hdmi_hs		;
				hdmi_in_vs_r	<=  hdmi_vs		;
				hdmi_in_de_r	<=  hdmi_de		;
			end
	end
	
reg [7:0] VGA_R,VGA_G,VGA_B;//延迟一个周期
always @(posedge hdmi_pclk or negedge s_rst_n)begin
	if(!s_rst_n) begin	
	      VGA_R <= 8'h00; //黑色
	      VGA_G <= 8'h00;
	      VGA_B <= 8'h00;		 
		end
	else if(hdmi_de && hdmi_vs_sel_r)begin
	       if(  (x_pixel > x_r)&&(x_pixel < x_r + w_r)&&(y_pixel == y_r)
	          ||(x_pixel > x_r)&&(x_pixel < x_r + w_r)&&(y_pixel == y_r + h_r)
	          ||(y_pixel > y_r)&&(y_pixel < y_r + h_r)&&(x_pixel == x_r)
	          ||(y_pixel > y_r)&&(y_pixel < y_r + h_r)&&(x_pixel == x_r + w_r)
	          )begin
	               VGA_R <= 8'b11111111;
                   VGA_G <= 8'h00;
                   VGA_B <= 8'h00;
	           end
	       else begin
	               VGA_R <= hdmi_in_data_r[7:0];
	               VGA_G <= hdmi_in_data_r[15:8];
	               VGA_B <= hdmi_in_data_r[23:16];
                end
	end
	else begin
	   	           VGA_R <= hdmi_in_data_r[7:0];
	               VGA_G <= hdmi_in_data_r[15:8];
	               VGA_B <= hdmi_in_data_r[23:16];
	end
end

reg de_r,hs_r,vs_r;
always @(posedge hdmi_pclk or negedge s_rst_n)begin
	if(!s_rst_n) begin	
        hs_r <= 'b0;
        vs_r <= 'b0;
        de_r <= 'b0;
    end
    else begin
        hs_r  <= ~hdmi_in_hs_r;
        vs_r  <= ~hdmi_in_vs_r;
        de_r  <= hdmi_in_de_r;
    end
end

assign hdmi_out_hs = hs_r;
assign hdmi_out_vs = vs_r;
assign hdmi_out_de = de_r;
assign hdmi_out_data = {VGA_B,VGA_G,VGA_R};
endmodule
