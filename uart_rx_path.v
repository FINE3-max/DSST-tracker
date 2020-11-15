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
    parameter BAUD_DIV       =14'd10416	;//������ʱ�ӣ�9600bps��100Mhz/9600=10416
    parameter BAUD_DIV_CAP   =14'd5208	;
	reg		[13:0] 	baud_div	= 'd0	;		//���������ü�����
	reg 			baud_bps	= 'd0	;		//���ݲ������ź�
	reg 			bps_start	= 'd0	;		//������������־


	always@(posedge iclk) begin
		if(baud_div == BAUD_DIV_CAP) begin		//�������ʼ�����������������ʱ�����������ź�baud_bps
			baud_bps <= 1'b1;
			baud_div <= baud_div+1'b1;
		end
		else if(baud_div < BAUD_DIV && bps_start) begin//�������ʼ���������ʱ���������ۼ�
			baud_div <= baud_div+1'b1;
			baud_bps <= 0;
		end
		else begin
			baud_bps <= 0;
			baud_div <= 0;
		end
	end

	reg		[ 4:0]	uart_rx_i_r	= 5'b11111;			//���ݽ��ջ�����

	always@(posedge iclk) begin
		uart_rx_i_r <= {uart_rx_i_r[3:0],uart_rx_i};
	end

	//���ݽ��ջ����������������յ�����͵�ƽʱ����uart_rx_int=0ʱ����Ϊ���յ���ʼ�ź�
	wire 			uart_rx_int			=	uart_rx_i_r[4] | uart_rx_i_r[3] | uart_rx_i_r[2] | uart_rx_i_r[1] | uart_rx_i_r[0];

	reg		[3:0] 	bit_num				=	0	;//�������ݸ���������
	reg 			uart_rx_done_r		=	0	;//���ݽ�����ɼĴ���
	reg 			state				=	1'b0;
	
	reg 	[7:0] 	uart_rx_data_o_r0	=	0	;//���ݽ��չ����У����ݻ�����
	reg 	[7:0] 	uart_rx_data_o_r1	=	0	;//���ݽ�����ɣ����ݼĴ���

	always@(posedge iclk) begin
		uart_rx_done_r<=1'b0;
		
		case(state)
			1'b0 : 
				if(!uart_rx_int) begin//���������յ�����͵�ƽʱ����uart_rx_int=0ʱ����Ϊ���յ���ʼ�źţ�����������ʱ��
					bps_start	<=	1'b1;
					state		<=	1'b1;
				end
			
			1'b1 :			
				if(baud_bps) begin	//ÿ�εȴ������ʲ�������ʱ���������ݣ��������ݻ�������
					bit_num	<=	bit_num+1'b1;
					
					if(bit_num < 4'd9) begin	//����1bit��ʼ�źţ�8bit��Ч�źţ�1bit�����ź�
						uart_rx_data_o_r0[bit_num-1]<=uart_rx_i;
					end
				end
				
				else if(bit_num == 4'd10) begin	//�������ʱ�򣬽������ݸ������������㣬����������ɱ�־λ����������д�����ݼĴ������رղ�����ʱ��
					bit_num				<=	0					;
					uart_rx_done_r		<=	1'b1				;
					uart_rx_data_o_r1	<=	uart_rx_data_o_r0	;
					state				<=	1'b0				;//����״̬0���ٴ�ѭ�����
					bps_start			<=	0					;
				end
			default:;
		endcase
	end

	assign uart_rx_data_o	=	uart_rx_data_o_r1	;		
	assign uart_rx_done		=	uart_rx_done_r		;
endmodule
