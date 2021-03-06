#include "fs.h"
#include "ime_dialog.h"
#include "main.h"
#include "net.h"
#include "utils.h"
#include "weather.h"

void initWeatherData()
{
	char buffer[1024], buffer2[2];
	
	if (!(dirExists("ux0:/data/VITAforecast")))
	{
		SceUID dir;
		
		dir = sceIoDopen("ux0:/data/VITAforecast");
		sceIoMkdir("ux0:/data/VITAforecast", 0777);
		sceIoDclose(dir);
	}
	
	if (fileExists("ux0:data/VITAforecast/zipCode.txt"))
		readFile("ux0:data/VITAforecast/zipCode.txt", zip, 12);
	else
	{
		strcpy(zip, keyboard_vita_get("Enter zip and country ID, e.g: '07001,us'", 12));
		writeFile("ux0:data/VITAforecast/zipCode.txt", zip, 12);
	}
	
	vita2d_start_drawing();
	vita2d_clear_screen();
	downloadWeatherData();
	endDrawing();
	
	if (fileExists("ux0:data/VITAforecast/weather.txt"))
		readFile("ux0:data/VITAforecast/weather.txt", buffer, 1024);
	
	if (fileExists("ux0:data/VITAforecast/metricUnits.txt"))
	{
		readFile("ux0:data/VITAforecast/metricUnits.txt", buffer2, 2);
		sscanf(buffer2, "%d", &CorF);
	}
	else
	{
		if (strcmp((getWeatherContent(buffer, "country\":\"", "\",\"sunrise")), "US") == 0)
		{
			sprintf(buffer2, "0");
			writeFile("ux0:data/VITAforecast/metricUnits.txt", buffer2, 2);
			sscanf(buffer2, "%d", &CorF);
		}
		else
		{
			sprintf(buffer2, "1");
			writeFile("ux0:data/VITAforecast/metricUnits.txt", buffer2, 2);
			sscanf(buffer2, "%d", &CorF);
		}
	}
}

char * urlConcat(char url[], char zipCode[], char apiKey[])
{
	static char str[100];
	strcpy(str, url);
	strcat(str, zipCode); 
	strcat(str, apiKey); 
	
	return str;
}

int downloadWeatherData()
{
	char url[100];
	strcpy(url, urlConcat("http://api.openweathermap.org/data/2.5/weather?zip=", zip, "&appid=1fbac727d98de6617dc510dd3b4aac4b"));
	while(1)
	{
		
		download(url, "ux0:data/VITAforecast/weather.txt");
		return 1;
	}
}

char * getWeatherContent(char * string, char * str1, char * str2) //Basically finds a string between two othe strings :D
{
	static char str[1024];
	char *begin, *end, *crawl;
	int i = 0;
	
	begin = strstr(string, str1);
	begin+=strlen(str1);
	
	end = strstr(string, str2);	
	
	for(crawl = begin; crawl < end; crawl++)
		str[i++] = *crawl;
		
	str[i] = '\0'; //add '\0' to the array so that it's a string!
	
	return str;
}

double strToDouble(char * str)
{
	double val = 0.0;
	
	sscanf(str, "%lf", &val);
	
	return val;
}

double kelvinToC(double temp)
{
	double c = (temp - 273);
	
	return c;
}	

double cToF(double temp)
{
	double c = (temp - 273);
	double f = (c * (1.8)) + 32.0;
	
	return f;
}	

double mToKm(double m)
{
	double km = (m * 3.6);
	return km;
}

double mToMph(double m)
{
	double mph = (m * 2.23694);
	return mph;
}

bool isDay()//basic assumption really
{
	SceDateTime time;
	sceRtcGetCurrentClockLocalTime(&time);
		
	if ((time.hour >= 6) && (time.hour <= 18))
		return true; //Day
	else 
		return false; //Night
}

void displayWeatherContent()
{
	char buffer[1024];
	
	double c = 0.0, cMin = 0.0, cMax = 0.0, windSpeed = 0.0;
	
	readFile("ux0:data/VITAforecast/weather.txt", buffer, 1024);
	
	if((getFileSize("ux0:data/VITAforecast/weather.txt")) <= 0)
	{
		vita2d_pvf_draw_textf(font, 30, 80, RGBA8(255, 255, 255, 255), 1.5f, "Error receiving data. The data could not be downloaded.");
		vita2d_pvf_draw_textf(font, 30, 120, RGBA8(255, 255, 255, 255), 1.5f, "Please ensure that you have a working net connection.");
	}
	else if (strcmp(buffer, "{\"cod\":\"500\",\"message\":\"Internal error: 500000\"}") == 0)
	{
		vita2d_pvf_draw_textf(font, 30, 80, RGBA8(255, 255, 255, 255), 1.5f, "Error parsing data. Zip code cannot be validated.");
		vita2d_pvf_draw_textf(font, 30, 120, RGBA8(255, 255, 255, 255), 1.5f, "Press square and renter zip and country code.");
	}
	else
	{
		displayWeatherIcon(getWeatherContent(buffer, "main\":\"", "\",\"des"), isDay());
	
		vita2d_pvf_draw_textf(font, 30, 80, RGBA8(255, 255, 255, 255), 2.0f, "%s", getWeatherContent(buffer, "name\":\"", "\",\"cod"));
	
		c = strToDouble(getWeatherContent(buffer, "temp\":", ",\"pressure"));
		cMin = strToDouble(getWeatherContent(buffer, "temp_min\":", ",\"temp_max"));
		cMax = strToDouble(getWeatherContent(buffer, "temp_max\":", "},\"visibility"));
		windSpeed = strToDouble(getWeatherContent(buffer, "speed\":", ",\"deg"));
	
		if (CorF == 0)
		{
			vita2d_pvf_draw_textf(font, 30, 160, RGBA8(255, 255, 255, 255), 3.0f, "%.0lf°F", cToF(c));
			vita2d_pvf_draw_textf(font, 210, 135, RGBA8(255, 255, 255, 255), 1.1f, "Min: %.0lf°F", cToF(cMin));
			vita2d_pvf_draw_textf(font, 210, 165, RGBA8(255, 255, 255, 255), 1.1f, "Max: %.0lf°F", cToF(cMax));
			vita2d_pvf_draw_textf(font, 30, 350, RGBA8(255, 255, 255, 255), 1.5f, "Wind: %.02f mph", mToMph(windSpeed));
			
			if (pressed & SCE_CTRL_SELECT)
			{
				CorF = 1;
				writeFile("ux0:data/VITAforecast/metricUnits.txt", "1", 2);
			}
		}
		else
		{
			vita2d_pvf_draw_textf(font, 30, 160, RGBA8(255, 255, 255, 255), 3.0f, "%.0lf°C", kelvinToC(c));
			vita2d_pvf_draw_textf(font, 210, 135, RGBA8(255, 255, 255, 255), 1.1f, "Min: %.0lf°C", kelvinToC(cMin));
			vita2d_pvf_draw_textf(font, 210, 165, RGBA8(255, 255, 255, 255), 1.1f, "Max: %.0lf°C", kelvinToC(cMax));
			vita2d_pvf_draw_textf(font, 30, 350, RGBA8(255, 255, 255, 255), 1.5f, "Wind %.02f km/h", mToKm(windSpeed)); 
			
			if (pressed & SCE_CTRL_SELECT)
			{
				CorF = 0;
				writeFile("ux0:data/VITAforecast/metricUnits.txt", "0", 2);
			}
		}
		
		vita2d_pvf_draw_textf(font, 30, 230, RGBA8(255, 255, 255, 255), 1.5f, "%s\n", getWeatherContent(buffer, "description\":\"", "\",\"icon"));
		vita2d_pvf_draw_textf(font, 30, 270, RGBA8(255, 255, 255, 255), 1.5f, "Humidity: %s%%", getWeatherContent(buffer, "humidity\":", ",\"temp_"));
		vita2d_pvf_draw_textf(font, 30, 310, RGBA8(255, 255, 255, 255), 1.5f, "Cloudiness: %s%%", getWeatherContent(buffer, "clouds\":{\"all\":", "},\"dt"));
		vita2d_pvf_draw_textf(font, 30, 390, RGBA8(255, 255, 255, 255), 1.5f, "Atmospheric pressure: %s hPa", getWeatherContent(buffer, "pressure\":", ",\"humidity"));
	}
}

void displayWeatherIcon(char * desc, bool day) //Uses main description
{
	if (day == true)
	{
		if (strcmp(desc, "Clear") == 0)
			vita2d_draw_texture(dClearSky, 330, 40);
		else if (strcmp(desc, "Clouds") == 0)
			vita2d_draw_texture(dClouds, 330, 40);
		else if ((strcmp(desc, "Rain") == 0) || (strcmp(desc, "Drizzle") == 0))
			vita2d_draw_texture(dRain, 330, 40);
		else if ((strcmp(desc, "Thunderstorm") == 0) || (strcmp(desc, "Extreme") == 0))
			vita2d_draw_texture(dThunderstorm, 330, 40);
		else if (strcmp(desc, "Snow") == 0)
			vita2d_draw_texture(dSnow, 330, 40);
		else if (strcmp(desc, "Atmosphere") == 0)
			vita2d_draw_texture(mist, 330, 40);
	}
	else
	{
		if (strcmp(desc, "Clear") == 0)
			vita2d_draw_texture(nClearSky, 330, 40);
		else if (strcmp(desc, "Clouds") == 0)
			vita2d_draw_texture(nClouds, 330, 40);
		else if ((strcmp(desc, "Rain") == 0) || (strcmp(desc, "Drizzle") == 0))
			vita2d_draw_texture(nRain, 330, 40);
		else if ((strcmp(desc, "Thunderstorm") == 0) || (strcmp(desc, "Extreme") == 0))
			vita2d_draw_texture(nThunderstorm, 330, 40);
		else if (strcmp(desc, "Snow") == 0)
			vita2d_draw_texture(nSnow, 330, 40);
		else if (strcmp(desc, "Atmosphere") == 0)
			vita2d_draw_texture(mist, 330, 40);
	}
}