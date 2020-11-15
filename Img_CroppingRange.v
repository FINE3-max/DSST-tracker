`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 16:57:31
// Design Name: 
// Module Name: Img_CroppingRange
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


module Img_CroppingRange(
    input wire clk,
    input wire rst_n,
    input wire IMG_CroppingRange_start,
    input wire [15:0] mid_pos_x,
    input wire [15:0] mid_pos_y,
    
    output  wire [15:0] hls_pos_x,//在搜索框中的坐标最小为（0，0）
    output	wire [15:0]	hls_pos_y,//输出hls模块中心相对坐标
    
	output	wire [11:0]	pixel_x_start,//在原图中的左上角坐标和右下角坐标
	output	wire [11:0]	pixel_x_end,
	output	wire [11:0]	pixel_y_start,
	output	wire [11:0]	pixel_y_end 
    );
parameter IMG_width = 1280;
parameter IMG_height = 720;
//parameter IMG_width = 640;
//parameter IMG_height = 480;

parameter Kuang_width = 300;
parameter Kuang_height = 150;

parameter Kuang_mid_x = 150;
parameter Kuang_mid_y = 75;

parameter Kuang_x_end = 1130;//1280-150=1130//640-150=490
parameter Kuang_y_end = 645;//720-75=645//480-75=405

reg	IMG_CroppingRange_start_r = 'd0;
reg	IMG_CroppingRange_start_r1 = 'd0;

reg [15:0] mid_pos_x_r ='d0;
reg [15:0] mid_pos_y_r ='d0;
reg	[15:0] hls_pos_x_r = 'd0;
reg	[15:0] hls_pos_y_r = 'd0;
reg	[11:0] pixel_x_start_r = 'd0;
reg	[11:0] pixel_x_end_r = 'd0;
reg	[11:0] pixel_y_start_r = 'd0;
reg	[11:0] pixel_y_end_r = 'd0;

always @(posedge clk or negedge rst_n)begin
	if(!rst_n) begin
		IMG_CroppingRange_start_r1 	<= 'd0;
		mid_pos_x_r 	<= 'd0;
		mid_pos_y_r 	<= 'd0;
	end
	else begin
		if(IMG_CroppingRange_start) begin
			IMG_CroppingRange_start_r1 	<= 'd1			;
			mid_pos_x_r 				<= 	mid_pos_x	;
			mid_pos_y_r 				<= 	mid_pos_y	;
			
		end
		else begin
			IMG_CroppingRange_start_r1 <= 'd0			;	
		end
	end
end

always @(posedge clk or negedge rst_n)begin
	if(!rst_n) begin
		IMG_CroppingRange_start_r 	<= 'd0;
	end
	else begin
		IMG_CroppingRange_start_r	<= IMG_CroppingRange_start_r1;
	end
end
/******************生成相对于原图的x坐标范围********************************************************/
always@(posedge clk or negedge rst_n)begin
    if(!rst_n)begin
		pixel_x_start_r 	<= 'd0;
		pixel_x_end_r 	<= 'd0;
	end
	else begin
       if(IMG_CroppingRange_start_r) begin
   	      if(mid_pos_x_r < Kuang_mid_x) begin
   		       pixel_x_start_r 	<= 'd1;
   		       pixel_x_end_r	<= Kuang_width;
   	    end
   	    else if(mid_pos_x_r > Kuang_x_end) begin
   		       pixel_x_start_r 	<= 'd1281 - Kuang_width;
   		       pixel_x_end_r	<= 'd1280;
   	    end
    	else begin
   		       pixel_x_start_r 	<= mid_pos_x_r - Kuang_mid_x + 'd1;
   		       pixel_x_end_r	<= mid_pos_x_r + Kuang_mid_x;
    	end
	 end
    end
end
/*******************生成相对于原图的y坐标范围*******************************************************/
always @(posedge clk or negedge rst_n)begin
	if(!rst_n) begin
			pixel_y_start_r <= 'd0;
			pixel_y_end_r 	<= 'd0;
		end
	else begin
		if(IMG_CroppingRange_start_r) begin
			if(mid_pos_y_r < Kuang_mid_y) begin
				pixel_y_start_r <= 'd0;
				pixel_y_end_r	<= Kuang_height -'d1;
			end
		    else if(mid_pos_y_r > Kuang_y_end) begin
					pixel_y_start_r <= 'd720 - Kuang_height;
					pixel_y_end_r	<= 'd719;
			end
		    else begin
					pixel_y_start_r <= mid_pos_y_r - Kuang_mid_y;
					pixel_y_end_r	<= mid_pos_y_r + Kuang_mid_y - 'd1;
			end
		end
	end
end
/**********************生成相对于搜索框的x坐标**********************************************************/
always @(posedge clk or negedge rst_n)begin
if(!rst_n) begin
	hls_pos_x_r <= 'd0;
end
else begin
	if(IMG_CroppingRange_start_r) begin
		if(mid_pos_x_r < Kuang_mid_x) begin
				hls_pos_x_r <= mid_pos_x_r - 'd1;
		end
		else if(mid_pos_x_r > Kuang_x_end) begin
				hls_pos_x_r <= mid_pos_x + Kuang_width -'d1280 - 'd1;
		end
		else begin
				hls_pos_x_r <= Kuang_mid_x - 'd1 ;
			 end
		end
	end
end
/*******************生成相对于搜索框的y坐标**********************************************************/
always @(posedge clk or negedge rst_n)begin
if(!rst_n) begin
	hls_pos_y_r <= 'd0;
end
else begin
    if(IMG_CroppingRange_start_r) begin
		if(mid_pos_y_r < Kuang_mid_y) begin
				hls_pos_y_r <= mid_pos_y_r - 'd1;
	    end
		else if(mid_pos_y_r > Kuang_y_end) begin
				hls_pos_y_r <= mid_pos_y + Kuang_height -'d720 - 'd1;
		end
		else begin
				hls_pos_y_r <= Kuang_mid_y - 'd1 ;
		     end
	    end
    end
end

assign	hls_pos_x		= hls_pos_x_r		;				
assign  hls_pos_y		= hls_pos_y_r		;

assign  pixel_x_start	= pixel_x_start_r	;
assign  pixel_x_end		= pixel_x_end_r		;
assign  pixel_y_start	= pixel_y_start_r	;
assign  pixel_y_end 	= pixel_y_end_r 	;
endmodule

