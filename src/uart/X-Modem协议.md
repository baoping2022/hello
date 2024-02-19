# XModem Protocol with CRC

## 介绍

Xmodem 协议是多年前创建的，是一种让两台计算机相互通信的简单方法。 凭借其半双工操作模式、128 字节数据包、ACK/NACK 响应和 CRC 数据检查，Xmodem 协议已进入许多应用领域。 事实上，当今 PC 上的大多数通信包都具有可供用户使用的 Xmodem 协议。

## 操作原理

Xmodem是一种半双工通信协议。接收方接收到数据包后，会发送一个确认(ACK)或不确认(NAK)的响应。原始协议的CRC部分使用更强大的16位CRC来验证数据块，并在此处使用。可以将Xmodem视为接收方驱动的协议。也就是说，接收方会向发送方发送一个初始字符“C”，表示它准备在CRC模式下接收数据。发送方随后发送一个133字节的数据包，接收方验证后会回复ACK或NAK，这时发送方要么发送下一个数据包，要么重新发送上一个数据包。这个过程会持续进行，直到接收方收到结束传输符（EOT）并正确向发送方发送ACK。初始握手后，接收方通过发送ACK或NAK来控制数据流向发送方。

Table 1. Xmodem CRC Packet Format

| Byte 1          | Byte 2        | Byte 3          | Bytes 4-131 | Bytes 132-133 |
| --------------- | ------------- | --------------- | ----------- | ------------- |
| Start of Header | Packet Number | (Packet Number) | Packet Data | 16-bit CRC    |

## 定义

以下定义用于协议流控制。

| Symbol | Description                                          | Value |
| ------ | ---------------------------------------------------- | ----- |
| SOH    | Start of Header                                      | 0x01  |
| EOT    | End of Transmission                                  | 0x04  |
| ACK    | Acknowledge                                          | 0x06  |
| NAK    | Not Acknowledge                                      | 0x15  |
| ETB    | End of Transmission Block (Return to Amulet OS mode) | 0x17  |
| CAN    | Cancel (Force receiver to start sending C's)         | 0x18  |
| C      | ASCII “C”                                            | 0x43  |

XmodemCRC数据包的第一个字节只能是SOH、EOT、CAN或ETB，其他任何值都是错误的。第二个和第三个字节构成一个带有校验和的数据包编号，将这两个字节相加，结果应该始终等于0xff。请注意，数据包编号从1开始，并在接收超过255个数据包时回滚为0。字节4至131构成数据包，并可以是任意值。字节132和133构成16位CRC校验码，CRC的高字节位于字节132处。CRC仅计算数据包字节（4至131）的值。

## 同步

接收方首先向发送方发送一个ASCII字符“C”（0x43），表示它希望使用CRC方法进行块验证。发送完初始的“C”后，接收方等待3秒（uboot中的实现定义为2秒）超时或直到缓冲区满标志被设置。如果接收方超时，则向发送方发送另一个“C”，并重新开始3秒（uboot中的实现定义为2秒）超时计时。这个过程一直持续，直到接收方接收到一个完整的133字节的数据包。

## 接收器的考虑事项

该协议在以下情况下发出NAK信号：

1. 任何字节的帧错误
2. 任何字节的溢出错误
3. 重复数据包
4. CRC错误
5. 接收超时（未在1秒内接收到数据包）在任何NAK情况下，发送者将重新传输上一个数据包。

事项1和2应被视为严重的硬件故障。请验证发送方和接收方是否使用相同的波特率、起始位和停止位。

事项3通常是发送方接收到失序的ACK信号并重新传输数据包。

事项4在嘈杂的环境中常见。

最后一项问题应在接收方发送NAK信号后自动纠正。

![xmodem](https://github.com/baoping2022/hello/tree/main/image/xmodem.png)

## crc 计算代码示例

```c
int calcrc(char *ptr, int count)
{
    int  crc;
    char i;
    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}
```

## 示例代码

针对x-modem协议的实现。

note

**getc : 从串口读取一个字节，带有一个超时时间timeout**

**putchar : 向串口写一个字节的数据**

```c
#define PACKET_SIZE	128

/* Line control codes */
#define SOH			0x01	/* start of header */
#define ACK			0x06	/* Acknowledge */
#define NAK			0x15	/* Negative acknowledge */
#define CAN			0x18	/* Cancel */
#define EOT			0x04	/* end of text */

#define TO	10

int getc(int timeout)
{
    /* TODO */
    return 0;
}
int putchar(int c)
{
    /* TODO */
    return 0;
}

/*
 * int GetRecord(char , char *)
 *  This private function receives a x-modem record to the pointer and
 * returns non-zero on success.
 */
static int GetRecord(char blocknum, char *dest)
{
	int		size;
	int		ch;
	unsigned	chk, j;

	chk = 0;

	if ((ch = getc(TO)) == -1)
		goto err;
	if (ch != blocknum) 
		goto err;
	if ((ch = getc(TO)) == -1) 
		goto err;
	if (ch != (~blocknum & 0xff))
		goto err;
	
	for (size = 0; size < PACKET_SIZE; ++size) {
		if ((ch = getc(TO)) == -1)
			goto err;
		chk = chk ^ ch << 8;
		for (j = 0; j < 8; ++j) {
			if (chk & 0x8000)
				chk = chk << 1 ^ 0x1021;
			else
				chk = chk << 1;
		}
		*dest++ = ch;
	}

	chk &= 0xFFFF;

	if (((ch = getc(TO)) == -1) || ((ch & 0xff) != ((chk >> 8) & 0xFF)))
		goto err;
	if (((ch = getc(TO)) == -1) || ((ch & 0xff) != (chk & 0xFF)))
		goto err;
	putchar(ACK);

	return (1);
err:
	putchar(CAN);
	// We should allow for resend, but we don't.
	return (0);
}

/*
 * int xmodem_rx(char *)
 *  This global function receives a x-modem transmission consisting of
 * (potentially) several blocks.  Returns the number of bytes received or
 * -1 on error.
 */
int xmodem_rx(char *dest)
{
	int     starting, ch;
	char    packetNumber, *startAddress = dest;

	packetNumber = 1;
	starting = 1;

	while (1) {
		if (starting)
			putchar('C');
		if (((ch = getc(1)) == -1) || (ch != SOH && ch != EOT))
			continue;
		if (ch == EOT) {
			putchar(ACK);
			return (dest - startAddress);
		}
		starting = 0;
		// Xmodem packets: SOH PKT# ~PKT# 128-bytes CRC16
		if (!GetRecord(packetNumber, dest))
			return (-1);
		dest += PACKET_SIZE;
		packetNumber++;
	}

	// the loop above should return in all cases
	return (-1);
}
```