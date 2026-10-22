void UseMessage ()
{
	return [(__bridge NSString *)Ptr() cStringUsingEncoding : NSASCIIStringEncoding ];
}
