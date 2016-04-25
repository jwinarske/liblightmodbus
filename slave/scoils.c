#include "../modlib.h"
#include "../parser.h"
#include "stypes.h"

#include "scoils.h"

//Use external slave configuration
extern MODBUSSlaveStatus MODBUSSlave;

void MODBUSBuildResponse01( union MODBUSParser *Parser )
{
	//Response for master request01
	uint8_t FrameLength = 6 + ( ( ( *Parser ).Request01.CoilCount - 1 ) >> 3 );
	uint8_t i = 0;

	//Do not respond when frame is broadcasted
	if ( ( *Parser ).Base.Address == 0 ) return;

	union MODBUSParser *Builder = (union MODBUSParser *) malloc( FrameLength ); //Allocate memory for builder union
	memset( ( *Builder ).Frame, 0, FrameLength ); //Fill frame with zeros

	MODBUSSlave.Response.Frame = (uint8_t *) realloc( MODBUSSlave.Response.Frame, FrameLength ); //Reallocate response frame memory to needed memory
	memset( MODBUSSlave.Response.Frame, 0, FrameLength ); //Empty response frame

	//Set up basic response data
	( *Builder ).Response01.Address = MODBUSSlave.Address;
	( *Builder ).Response01.Function = ( *Parser ).Request01.Function;
	( *Builder ).Response01.BytesCount = 1 + ( ( ( *Parser ).Request01.CoilCount - 1 ) >> 3 );

	//Copy registers to response frame
	for ( i = 0; i < ( *Parser ).Request01.CoilCount; i++ )
		MODBUSWriteMaskBit( ( *Builder ).Response01.Values, 32, i, MODBUSReadMaskBit( MODBUSSlave.Coils, 1 + ( ( MODBUSSlave.CoilCount - 1 ) >> 3 ), i + ( *Parser ).Request01.FirstCoil ) );

	//Calculate CRC
	( *Builder ).Frame[FrameLength - 2] = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 ) & 0x00FF;
	( *Builder ).Frame[FrameLength - 1] = ( MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 ) & 0xFF00 ) >> 8;

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Builder ).Frame, FrameLength );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = FrameLength;

	//Free union memory
	free( Builder );
}

void MODBUSBuildResponse05( union MODBUSParser *Parser )
{
	//Response for master request05
	uint8_t FrameLength = 8;

	//Do not respond when frame is broadcasted
	if ( ( *Parser ).Base.Address == 0 ) return;

	union MODBUSParser *Builder = (union MODBUSParser *) malloc( FrameLength ); //Allocate memory for builder union
	memset( ( *Builder ).Frame, 0, FrameLength ); //Fill frame with zeros

	MODBUSSlave.Response.Frame = (uint8_t *) realloc( MODBUSSlave.Response.Frame, FrameLength ); //Reallocate response frame memory to needed memory
	memset( MODBUSSlave.Response.Frame, 0, FrameLength ); //Empty response frame

	//Set up basic response data
	( *Builder ).Response05.Address = MODBUSSlave.Address;
	( *Builder ).Response05.Function = ( *Parser ).Request05.Function;
	( *Builder ).Response05.Coil = MODBUSSwapEndian( ( *Parser ).Request05.Coil );
	( *Builder ).Response05.Value = MODBUSSwapEndian( ( *Parser ).Request05.Value );

	//Calculate CRC
	( *Builder ).Response05.CRC = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 );

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Builder ).Frame, FrameLength );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = FrameLength;

	//Free union memory
	free( Builder );
}

void MODBUSBuildResponse15( union MODBUSParser *Parser )
{
	//Response for master request15
	uint8_t FrameLength = 8;

	//Do not respond when frame is broadcasted
	if ( ( *Parser ).Base.Address == 0 ) return;

	union MODBUSParser *Builder = (union MODBUSParser *) malloc( FrameLength ); //Allocate memory for builder union
	memset( ( *Builder ).Frame, 0, FrameLength ); //Fill frame with zeros

	MODBUSSlave.Response.Frame = (uint8_t *) realloc( MODBUSSlave.Response.Frame, FrameLength ); //Reallocate response frame memory to needed memory
	memset( MODBUSSlave.Response.Frame, 0, FrameLength ); //Empty response frame

	//Set up basic response data
	( *Builder ).Response15.Address = MODBUSSlave.Address;
	( *Builder ).Response15.Function = ( *Parser ).Request15.Function;
	( *Builder ).Response15.FirstCoil = MODBUSSwapEndian( ( *Parser ).Request15.FirstCoil );
	( *Builder ).Response15.CoilCount = MODBUSSwapEndian( ( *Parser ).Request15.CoilCount
 );

	//Calculate CRC
	( *Builder ).Response15.CRC = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 );

	//Copy result from union to frame pointer
	memcpy( MODBUSSlave.Response.Frame, ( *Builder ).Frame, FrameLength );

	//Set frame length - frame is ready
	MODBUSSlave.Response.Length = FrameLength;

	//Free union memory
	free( Builder );
}

void MODBUSParseRequest01( union MODBUSParser *Parser )
{
	//Read multiple coils
	//Using data from union pointer

	//Update frame length
	uint8_t FrameLength = 8;

	//Check frame CRC
	if ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) != ( *Parser ).Request01.CRC ) return;

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request01.FirstCoil = MODBUSSwapEndian( ( *Parser ).Request01.FirstCoil );
	( *Parser ).Request01.CoilCount = MODBUSSwapEndian( ( *Parser ).Request01.CoilCount );

	//Check if coil is in valid range
	if ( ( *Parser ).Request01.CoilCount == 0 )
	{
		//Illegal data value error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x01, 0x03 );
		return;
	}

	if ( ( *Parser ).Request01.CoilCount > MODBUSSlave.CoilCount )
	{
		//Illegal data address error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x01, 0x02 );
		return;
	}

	if ( ( *Parser ).Request01.FirstCoil >= MODBUSSlave.CoilCount || (uint32_t) ( *Parser ).Request01.FirstCoil + (uint32_t) ( *Parser ).Request01.CoilCount > (uint32_t) MODBUSSlave.CoilCount )
	{
		//Illegal data address exception
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x01, 0x02 );
		return;
	}

	//Respond
	MODBUSBuildResponse01( Parser );
}

void MODBUSParseRequest05( union MODBUSParser *Parser )
{
	//Write single coil
	//Using data from union pointer

	//Update frame length
	uint8_t FrameLength = 8;

	//Check frame CRC
	if ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) != ( *Parser ).Request01.CRC ) return;

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request05.Coil = MODBUSSwapEndian( ( *Parser ).Request05.Coil );
	( *Parser ).Request05.Value = MODBUSSwapEndian( ( *Parser ).Request05.Value );

	//Check if coil is in valid range
	if ( ( *Parser ).Request05.Coil >= MODBUSSlave.CoilCount )
	{
		//Illegal data address error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x05, 0x02 );
		return;
	}

	//Check if coil value is a valid value
	if ( ( *Parser ).Request05.Value != 0x0000 && ( *Parser ).Request05.Value != 0xFF00 )
	{
		//Illegal data address error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x05, 0x03 );
		return;
	}

	MODBUSWriteMaskBit( MODBUSSlave.Coils, 1 + ( ( MODBUSSlave.CoilCount - 1 ) << 3 ), ( *Parser ).Request05.Coil, ( *Parser ).Request05.Value == 0xFF00 );

	//Respond
	MODBUSBuildResponse05( Parser );
}

void MODBUSParseRequest15( union MODBUSParser *Parser )
{
	//Write multiple coils
	//Using data from union pointer

	//Update frame length
	uint8_t i = 0;
	uint8_t FrameLength = 9 + ( *Parser ).Request15.BytesCount;

	//Check frame CRC
	//Shifting is used instead of dividing for optimisation on smaller devices (AVR)
	if ( ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) & 0x00FF ) != ( *Parser ).Request15.Values[( *Parser ).Request15.BytesCount] ) return;
	if ( ( ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) & 0xFF00 ) >> 8 ) != ( *Parser ).Request15.Values[( *Parser ).Request15.BytesCount + 1] ) return;


	//Check if bytes or registers count isn't 0
	if ( ( *Parser ).Request15.BytesCount == 0 || ( *Parser ).Request15.CoilCount == 0 )
	{
		//Illegal data value error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x0F, 0x03 );
		return;
	}

	//Swap endianness of longer members (but not CRC)
	( *Parser ).Request15.FirstCoil = MODBUSSwapEndian( ( *Parser ).Request15.FirstCoil );
	( *Parser ).Request15.CoilCount = MODBUSSwapEndian( ( *Parser ).Request15.CoilCount );

	//Check if bytes count matches coils count
	if ( 1 + ( ( ( *Parser ).Request15.CoilCount - 1 ) >> 3 )  != ( *Parser ).Request15.BytesCount )
	{
		//Illegal data value error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x0F, 0x03 );
		return;
	}

	//Check if registers are in valid range
	if ( ( *Parser ).Request15.CoilCount > MODBUSSlave.CoilCount )
	{
		//Illegal data address error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x0F, 0x02 );
		return;
	}

	if ( ( *Parser ).Request15.FirstCoil >= MODBUSSlave.CoilCount || (uint32_t) ( *Parser ).Request15.FirstCoil + (uint32_t) ( *Parser ).Request15.CoilCount > (uint32_t) MODBUSSlave.CoilCount )
	{
		//Illegal data address error
		if ( ( *Parser ).Base.Address != 0 ) MODBUSBuildException( 0x0F, 0x02 );
		return;
	}

	//Write values to registers
	for ( i = 0; i < ( *Parser ).Request15.CoilCount; i++ )
		MODBUSWriteMaskBit( MODBUSSlave.Coils, MODBUSSlave.CoilCount, ( *Parser ).Request15.FirstCoil + i, MODBUSReadMaskBit( ( *Parser ).Request15.Values, ( *Parser ).Request15.BytesCount, i ) );

	//Respond
	MODBUSBuildResponse15( Parser );
}
