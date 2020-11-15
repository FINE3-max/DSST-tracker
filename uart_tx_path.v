`timescale 1ns / 1ps

module uart_tx_path(
	input 			iclk,

	input	[7:0] 	uart_tx_data_i	,	//����������
	input			uart_tx_en_i	,	//���ͷ���ʹ���ź�
	
	output 			uart_tx_o		,
	output			uart_tx_done
);
    parameter BAUD_DIV       =14'd10416	;//������ʱ�ӣ�9600bps��100Mhz/9600=10416
    parameter BAUD_DIV_CAP   =14'd5208	;

	reg 	[13:0] 	baud_div		= 0				;//���������ü�����
	reg 			baud_bps		= 0				;//���ݷ��͵��ź�,����Ч
	reg 	[9:0] 	send_data		= 10'b1111111111;//���������ݼĴ�����1bit��ʼ�ź�+8bit��Ч�ź�+1bit�����ź�
	reg		[3:0] 	bit_num			= 0				;//�������ݸ���������
	reg 			uart_send_flag	= 0				;//���ݷ��ͱ�־λ
	reg 			uart_tx_o_r		= 1				;//�������ݼĴ�������ʼ״̬λ��
	reg				uart_tx_done_r	= 0				;

	always@(posedge iclk) begin
		if(baud_div== BAUD_DIV_CAP) begin//�������ʼ��������������ݷ����е�ʱ�����������ź�baud_bps��������������
				baud_bps <= 1'b1;
				baud_div <= baud_div + 1'b1;
		end
		else if(baud_div< BAUD_DIV && uart_send_flag) begin//���ݷ��ͱ�־λ��Ч�ڼ䣬�����ʼ������ۼӣ��Բ���������ʱ��
				baud_div <= baud_div + 1'b1;
				baud_bps <= 0;	
		end
		else begin
				baud_bps <= 0;
				baud_div <= 0;
		end
	end

	always@(posedge iclk) begin
		if(uart_tx_en_i) begin	//�������ݷ���ʹ���ź�ʱ���������ݷ��ͱ�־�ź�
			uart_send_flag	<=1'b1;
			send_data		<={1'b1,uart_tx_data_i,1'b0};//���������ݼĴ���װ�1bit��ʼ�ź�0+8bit��Ч�ź�+1bit�����ź�
		end
		
		else if((bit_num == 4'd10)&&(baud_div == BAUD_DIV)) begin//���ͽ���ʱ��������ͱ�־�źţ���������������ݼĴ����ڲ��ź�
			uart_send_flag	<= 1'b0;
			send_data		<= 10'b1111_1111_11;
		end
	end

	//���ͽ�����ʾ
	always@(posedge iclk) begin
		if((bit_num == 4'd10)&&(baud_div == BAUD_DIV)) begin
			uart_tx_done_r <= 1;
		end
		else begin
			uart_tx_done_r <= 0;
		end
	end

	always@(posedge iclk) begin
		if(uart_send_flag) begin	//������Чʱ��
			if(baud_bps) begin//��ⷢ�͵��ź�
				if(bit_num<=4'd9) begin
					uart_tx_o_r	<= 	send_data[bit_num];	//���ʹ����ͼĴ��������ݣ��ӵ�λ����λ
					bit_num		<=	bit_num+1'b1;
				end
			end
			else if((bit_num == 4'd10)&&(baud_div == BAUD_DIV)) begin
				bit_num <= 4'd0;
			end
		end
		else begin
			uart_tx_o_r	<= 1'b1;	//����״̬ʱ�����ַ��Ͷ�λ�ߵ�ƽ���Ա�����ʱ������͵�ƽ�ź�
			bit_num		<= 0;
		end
	end

	assign uart_tx_o	=	uart_tx_o_r		;
	assign uart_tx_done	=	uart_tx_done_r	;

endmodule