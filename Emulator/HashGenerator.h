#pragma once


class HashGen
{
public:

	  //Generate Hash using Adler
	 //https://en.wikipedia.org/wiki/Adler-32
	static unsigned int Adler(char* data, int len)
	{
		int a = 1, b = 0;
		for (int i = 0; i < len; ++i )
		{
			a = (a + data[i]) % MOD_ADLER;
			b = (b + a) % MOD_ADLER;
		}

		return (b << 16) | a;
	}

private:

	const static int MOD_ADLER = 65521;
};