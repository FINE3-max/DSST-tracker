`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 16:51:12
// Design Name: 
// Module Name: uart_rx_path
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


module uart_rx_path(
  	input 			iclk			,
	input 			uart_rx_i		,
	
	output [7:0] 	uart_rx_data_o	,
	output 			uart_rx_done	
 );
    parameter BAUD_DIV       =14'd10416	;//波特率时钟，9600bps，100Mhz/9600=10416
    parameter BAUD_DIV_CAP   =14'd5208	;
	reg		[13:0] 	baud_div	= 'd0	;		//波特率设置计数器
	reg 			baud_bps	= 'd0	;		//数据采样点信号
	reg 			bps_start	= 'd0	;		//波特率启动标志


	always@(posedge iclk) begin
		if(baud_div == BAUD_DIV_CAP) begin		//当波特率计数器计数到采样点时，产生采样信号baud_bps
			baud_bps <= 1'b1;
			baud_div <= baud_div+1'b1;
		end
		else if(baud_div < BAUD_DIV && bps_start) begin//当波特率计数器启动时，计数器累加
			baud_div <= baud_div+1'b1;
			baud_bps <= 0;
		end
		else begin
			baud_bps <= 0;
			baud_div <= 0;
		end
	end

	reg		[ 4:0]	uart_rx_i_r	= 5'b11111;			//数据接收缓存器

	always@(posedge iclk) begin
		uart_rx_i_r <= {uart_rx_i_r[3:0],uart_rx_i};
	end

	//数据接收缓存器，当连续接收到五个低电平时，即uart_rx_int=0时，作为接收到起始信号
	wire 			uart_rx_int			=	uart_rx_i_r[4] | uart_rx_i_r[3] | uart_rx_i_r[2] | uart_rx_i_r[1] | uart_rx_i_r[0];

	reg		[3:0] 	bit_num				=	0	;//接收数据个数计数器
	reg 			uart_rx_done_r		=	0	;//数据接收完成寄存器
	reg 			state				=	1'b0;
	
	reg 	[7:0] 	uart_rx_data_o_r0	=	0	;//数据接收过程中，数据缓存器
	reg 	[7:0] 	uart_rx_data_o_r1	=	0	;//数据接收完成，数据寄存器

	always@(posedge iclk) begin
		uart_rx_done_r<=1'b0;
		
		case(state)
			1'b0 : 
				if(!uart_rx_int) begin//当连续接收到五个低电平时，即uart_rx_int=0时，作为接收到起始信号，启动波特率时钟
					bps_start	<=	1'b1;
					state		<=	1'b1;
				end
			
			1'b1 :			
				if(baud_bps) begin	//每次等待波特率采样中心时，接收数据，放入数据缓存器中
					bit_num	<=	bit_num+1'b1;
					
					if(bit_num < 4'd9) begin	//接收1bit起始信号，8bit有效信号，1bit结束信号
						uart_rx_data_o_r0[bit_num-1]<=uart_rx_i;
					end
				end
				
				else if(bit_num == 4'd10) begin	//接收完成时候，接收数据个数计数器清零，产生接收完成标志位，并将数据写入数据寄存器，关闭波特率时候
					bit_num				<=	0					;
					uart_rx_done_r		<=	1'b1				;
					uart_rx_data_o_r1	<=	uart_rx_data_o_r0	;
					state				<=	1'b0				;//进入状态0，再次循环检测
					bps_start			<=	0					;
				end
			default:;
		endcase
	end

	assign uart_rx_data_o	=	uart_rx_data_o_r1	;		
	assign uart_rx_done		=	uart_rx_done_r		;
endmodule
