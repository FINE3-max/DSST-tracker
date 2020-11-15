`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 16:52:00
// Design Name: 
// Module Name: uart_top
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


module uart_top(
    input	wire 			iclk			,
	input 	wire			s_rst_n			,
	
	//rx
	input 	wire			uart_rx_i		,
	
	output	wire			rx_over			,	//接收帧成功标志，为1时读出初始数据
	
	output	wire	[15:0]	initialpos_x	,	//初始信息		
	output	wire	[15:0]	initialpos_y	,	//初始信息
	output	wire	[15:0]	initialtarget_w	,	//初始信息
	output	wire	[15:0]	initialtarget_h	,	//初始信息
	
	//tx
	input	wire			tx_start		,	//发送开始信号，发送时拉高一个周期，
	input	wire	[15:0]	result_x		,	//发送开始时，此信号保持，坐标x
	input	wire	[15:0]	result_y		,   //发送开始时，此信号保持，坐标y
	input	wire 	[15:0]	result_w		,
	input	wire 	[15:0]	result_h		,
	
	output  wire			uart_tx_o
);

//rx信号	
	reg 	[15:0]	rx_x		= 0	;
	reg 	[15:0]	rx_y		= 0	;
	reg 	[15:0]	rx_w		= 0	;
	reg 	[15:0]	rx_h		= 0	;
	reg		[4:0]	rx_state	= 0	;
	
	reg				rx_success	= 0	;
	
	wire	[7:0] 	uart_rx_data_o	;
	wire 			uart_rx_done	;


	//接收帧	
	always@(posedge iclk) begin
		if(!s_rst_n) begin
			rx_x			<= 'd0;
			rx_y			<= 'd0;
			rx_w			<= 'd0;
			rx_h			<= 'd0;
			rx_state		<= 'd0;
		end
		
		else if(uart_rx_done && rx_state < 'd8)begin
        case(rx_state)
            'd0:begin
                rx_state <= 'd1;
                rx_x[15:8] <= uart_rx_data_o;
            end
            'd1:begin
                rx_state <= 'd2;
                rx_x[7:0] <= uart_rx_data_o;
            end
            'd2:begin
                rx_state <= 'd3;
                rx_y[15:8] <= uart_rx_data_o;
            end
            'd3:begin
                rx_state <= 'd4;
                rx_y[7:0] <= uart_rx_data_o;
            end
            'd4:begin
                rx_state <= 'd5;
                rx_w[15:8] <= uart_rx_data_o;
            end
            'd5:begin
                rx_state <= 'd6;
                rx_w[7:0] <= uart_rx_data_o;
            end
            'd6:begin
                rx_state <= 'd7;
                rx_h[15:8] <= uart_rx_data_o;
            end
            'd7:begin
                rx_state <= 'd8;
                rx_h[7:0] <= uart_rx_data_o;
            end
            default:;
        endcase
    end
    else if(rx_state == 'd8)begin
			rx_state <= 'd0;
	end
end

	//接收帧校验
always@(posedge iclk) begin
		if(!s_rst_n) begin
			rx_success 	<= 'd0;
		end
		else if(rx_state == 'd8) begin
				rx_success <= 'd1;
		end
		else begin
				rx_success 	<= 'd0;
		end
end

//tx信号
	reg				tx_start_r		= 'd0;
	reg		[15:0]	result_x_r		= 'd0;
	reg		[15:0]	result_y_r		= 'd0;
	reg		[15:0]	result_w_r		= 'd0;
	reg		[15:0]	result_h_r		= 'd0;

	reg		[4:0]	tx_state	= 0		;		
	reg				tx_en		= 0		;
	reg		[7:0]	tx_data		= 0		;
	
	wire			uart_tx_done		;
		
	//发送帧控制
	always@(posedge iclk) begin
		if(!s_rst_n) begin
			tx_start_r	<= 'd0;
		    result_x_r  <= 'd0;
		    result_y_r  <= 'd0;
		    result_w_r  <= 'd0;
		    result_h_r  <= 'd0;
		end
		else begin
			tx_start_r <= tx_start;
			if(tx_start) begin
				result_x_r	<= result_x;
				result_y_r	<= result_y;
				result_w_r  <= result_w;
				result_h_r  <= result_h;
			end
		end
	end
	
	//发送帧数据状态机
always@(posedge iclk) begin
		if(!s_rst_n) begin
			tx_state		<= 'd0	;
			tx_en			<= 'd0	;
			tx_data         <= 'd0	;
		end
		
		else begin
			case (tx_state) 
				'd0: begin						//接收成功正常发送
					if(tx_start_r) begin		//发送开始信号，发送时拉高一个周期
						tx_state 	<= 'd1;
						tx_en 		<= 'd1;
						tx_data		<= result_x_r[15:8];
					end
				end
				'd1: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd2;
					end
				end
				'd2: begin						//发送坐标x高8位		
					tx_state 	<= 'd3;
					tx_en 		<= 'd1;
					tx_data		<= result_x_r[7:0];
				end
				'd3: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd4;
					end
				end
				'd4: begin						//发送坐标x低8位
					tx_state 	<= 'd5;
					tx_en 		<= 'd1;
					tx_data		<= result_y_r[15:8];			
				end
				'd5: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd6;
					end
				end
				'd6: begin						//发送坐标y高8位
					tx_state 	<= 'd7;
					tx_en 		<= 'd1;
					tx_data		<= result_y_r[7:0];
				end
				'd7:  begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd8;
					end
				end
				'd8: begin						//发送坐标y低8位
					tx_state 	<= 'd9;
					tx_en 		<= 'd1;
					tx_data		<= result_w_r[15:8];		
				end
				'd9: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd10;
					end
				end
				'd10: begin						//发送帧计数高8位
					tx_state 	<= 'd11;
					tx_en 		<= 'd1;
					tx_data		<= result_w_r[7:0];
				end
				'd11: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd12;
					end
				end
				'd12: begin						//发送帧计数低8位
					tx_state 	<= 'd13;
					tx_en 		<= 'd1;
					tx_data		<= result_h_r[15:8];
				end
				'd13: begin
					tx_en <= 'd0;
					if(uart_tx_done) begin
						tx_state <= 'd14;
					end
				end
				'd14: begin						//发送校验码
					tx_state 	<= 'd15;
					tx_en 		<= 'd1;
					tx_data		<= result_h_r[7:0];
				end
				'd15: begin
					tx_state 	<= 'd0;
					tx_en 		<= 'd0;
					tx_data		<= 'd0;
				end
				default:;
			endcase
		end
	end
	
	assign rx_over				=	rx_success	;
	assign initialpos_x 		= 	rx_x		;
	assign initialpos_y 		=	rx_y		;
	assign initialtarget_w		=	rx_w		;
	assign initialtarget_h		=	rx_h		;

	uart_rx_path uart_rx_path_u (
		.iclk				(iclk			), 
		.uart_rx_i			(uart_rx_i		), 

		.uart_rx_data_o		(uart_rx_data_o	), 
		.uart_rx_done		(uart_rx_done	)
	);
	uart_tx_path uart_tx_path_u (
		.iclk				(iclk			), 
		.uart_tx_data_i		(tx_data		), 
		.uart_tx_en_i		(tx_en			), 
		.uart_tx_o			(uart_tx_o		),
		.uart_tx_done		(uart_tx_done	)
	);
endmodule
