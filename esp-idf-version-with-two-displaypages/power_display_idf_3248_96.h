#include <sstream>
#include <numeric>
#include "esphome.h"

// Global variables. Needed to retain values after reboot
double currentPower, currentPrice, todayMaxPrice, dailyEnergy, dailyCharge, tomorrowsMaxPrice, tomorrowsMinPrice, tomorrowsAverage;
std::string TodaysPrices, TomorrowsPrices;
	double priceArray[100];
	double priceArrayTomorrow[100];	

// PowerDisplay class:
class PowerDisplay : public Component {

public:

	void DisplayIcons (display::Display *buff, int x, int y) {
		if (currentPower >= 0)
			buff->image(x, y, &id(grid_power));
		else
			buff->image(x, y, &id(solar_power));
	}

	void CreateGraph (display::Display *buff, int x, int y, int width, int  height, Color color = COLOR_ON) {
		setPos(x, y);
		setWidth(width);
		setHeigh(height);		
		buff->rectangle(x, y, width, height, color);
	}

	void SetGraphScale (double xMin, double  xMax, double yMin) {		
		double  yMax = todayMaxPrice;
		xFactor = graphWidth / (xMax-xMin);
		xFactorGrid = graphWidth / (xMax/4-xMin);
		yFactor = graphHeight / (yMax-yMin);

	}
	
	void SetGraphScaleTomorrow (double xMin, double  xMax, double yMin) {		
		double  yMax = tomorrowsMaxPrice;
		xFactor = graphWidth / (xMax-xMin);
		xFactorGrid = graphWidth / (xMax/4-xMin);
		yFactor = graphHeight / (yMax-yMin);
	}

	void SetGraphGrid(display::Display *buff, double xLow, double xInterval, double yLow, double yInterval, Color color = COLOR_ON) {
		double xLabel=0, yLabel = 0;
		double i2=0;
		Color labelColor = COLOR_CSS_WHITESMOKE;
		for(double i=(xPos + xLow*xFactorGrid); i <= graphWidth+xPos; i+= xInterval*xFactorGrid) {
//			ESP_LOGD("GraphGrid i: ", String(i).c_str());
			buff->line(i, yPos, i, yPos+graphHeight, color);
			buff->printf(i-4, yPos+graphHeight+10, &id(small_text),labelColor , TextAlign::BASELINE_LEFT, "%.0f", xLabel);
			xLabel += xInterval;
			i2 += xInterval*xFactorGrid;
		}
		// For the last hour...
		buff->printf(i2+8, yPos+graphHeight+10, &id(small_text),labelColor , TextAlign::BASELINE_LEFT, "%.0f", xLabel);
		
		
		for(double j=(yLow*yFactor); j < graphHeight; j+= yInterval*yFactor) {
//			ESP_LOGD("GraphGrid j: ", String(j).c_str());
			buff->line(xPos, yPos+graphHeight-j, xPos+graphWidth, yPos+graphHeight-j, color);
			buff->printf(xPos-2, yPos+graphHeight-j, &id(small_text),labelColor , TextAlign::BASELINE_RIGHT, "%.1f", yLabel);
			yLabel += yInterval;
		}
	}
	
	// Functions to set the values from Home Assistant
	
	void SetCurrentPower(double power) {
		if (!isnan(power)) {
			currentPower = power;
		}
	}
	
	void SetCurrentPrice(double price) {
		if (!isnan(price)) {
			currentPrice = price;
		}
	}
	
	void SetTodayDailyCharge(double price) {
		if (!isnan(price)) {
			dailyCharge = price;
		}
	}

	void SetTodayMaxPrice(double price) {
		if (!isnan(price) || price == 0) {
			todayMaxPrice = price;
		}
	}
		
	void SetTodaysPrices(std::string prices) {
		if (prices != "") {
			TodaysPrices = prices;
		}
	}

	void SetTomorrowsPrices(std::string prices) {
			TomorrowsPrices = prices;
		}
		
	void WriteDailyEnergy(double energy) {
		if (!isnan(energy)) {
			dailyEnergy = energy;
		}
	}


	// Display current power usage
	void WritePowerText(display::Display *buff, int x, int y) {
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%.0f W", currentPower);		
	}

	// Display text at the top of the second page
	void WriteTomorrowText(display::Display *buff, int x, int y) {	
		buff->print(x, y, &id(large_text), COLOR_CSS_WHITESMOKE, TextAlign::BASELINE_CENTER, "I morgen");		
	}

	// Display current price and the price level
	void WritePriceText(display::Display *buff, int x, int y) {
		
		buff->printf(x, y-40, &id(price_text), COLOR_CSS_WHITESMOKE, TextAlign::BASELINE_CENTER, "%.2f kr/kWh", currentPrice);  // Display size dependent
		
		std::string price;
		if (inRange(currentPrice, id(EXTREMELY_EXPENSIVE).state, 100)) {price = id(EXTREMELY_EXPENSIVE_TEXT).state;}
		else if (inRange(currentPrice, id(VERY_EXPENSIVE).state, id(EXTREMELY_EXPENSIVE).state)){price = id(VERY_EXPENSIVE_TEXT).state;}
		else if (inRange(currentPrice, id(EXPENSIVE).state, id(VERY_EXPENSIVE).state)){price = id(EXPENSIVE_TEXT).state;}
		else if (inRange(currentPrice, id(NORMAL).state, id(EXPENSIVE).state)){price = id(NORMAL_TEXT).state;}
		else if (inRange(currentPrice, id(CHEAP).state, id(NORMAL).state)){price = id(CHEAP_TEXT).state;}
		else if (inRange(currentPrice, id(VERY_CHEAP).state, id(CHEAP).state)){price = id(VERY_CHEAP_TEXT).state;}
		else if (inRange(currentPrice, -100, id(VERY_CHEAP).state)){price = id(BELOW_VERY_CHEAP_TEXT).state;} 		
		
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%s", price.c_str());		
	}
	
	void ClearPriceText(display::Display *buff) {		// Display size dependant
		buff->filled_rectangle(1,457,238,22,COLOR_CSS_BLACK);   // x,y,width,height 
	}
	
	
	// Write the timeline on the graph
	void WriteTimeLine(display::Display *buff, double hour, double minute, Color color = COLOR_ON) {
		double timeLineVal = hour + (minute/60);
		buff->line(xPos + timeLineVal*xFactorGrid, yPos, xPos + timeLineVal*xFactorGrid, yPos+graphHeight, color);
	}
	// Write energy consumed so far today
	void WriteDailyAmount(display::Display *buff, int x, int y, Color color = COLOR_ON) {
		buff->printf(x, y, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Idag: %.1f kWh", dailyEnergy);   	
		buff->printf(x, y+23, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Samlet: %.2f kr", dailyCharge);	
	}

	// Write info about min, max and average price tomorrow
	void WritePriceInfo(display::Display *buff, int x, int y) {
		buff->printf(x, y, &id(energy_text), PriceColour(tomorrowsMinPrice), TextAlign::BASELINE_CENTER, "Min: %.2f kr/kWh", tomorrowsMinPrice);  
		buff->printf(x, y+23, &id(energy_text), PriceColour(tomorrowsMaxPrice), TextAlign::BASELINE_CENTER, "Max: %.2f kr/kWh", tomorrowsMaxPrice); 
		buff->printf(x, y+342, &id(price_text), PriceColour(tomorrowsAverage), TextAlign::BASELINE_CENTER, "Gennemsnit %.2f kr/kWh", tomorrowsAverage); 

		if (!PriceVectorTomorrow.empty()) {
			std::string price;
			if (inRange(tomorrowsAverage, id(EXTREMELY_EXPENSIVE).state, 100)) {price = id(EXTREMELY_EXPENSIVE_TEXT).state;}
			else if (inRange(tomorrowsAverage, id(VERY_EXPENSIVE).state, id(EXTREMELY_EXPENSIVE).state)){price = id(VERY_EXPENSIVE_TEXT).state;}
			else if (inRange(tomorrowsAverage, id(EXPENSIVE).state, id(VERY_EXPENSIVE).state)){price = id(EXPENSIVE_TEXT).state;}
			else if (inRange(tomorrowsAverage, id(NORMAL).state, id(EXPENSIVE).state)){price = id(NORMAL_TEXT).state;}
			else if (inRange(tomorrowsAverage, id(CHEAP).state, id(NORMAL).state)){price = id(CHEAP_TEXT).state;}
			else if (inRange(tomorrowsAverage, id(VERY_CHEAP).state, id(CHEAP).state)){price = id(VERY_CHEAP_TEXT).state;}
			else if (inRange(tomorrowsAverage, -100, id(VERY_CHEAP).state)){price = id(BELOW_VERY_CHEAP_TEXT).state;} 		
		
			// buff->printf(x, y+222, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%s", price.c_str());
		}

		if (PriceVectorTomorrow.empty()) {
			buff->print(x, y+200, &id(large_text), my_grey, TextAlign::BASELINE_CENTER, "Ingen data");	
			}
	}


	// Draw the graph
	void DrawPriceGraph (display::Display *buff) {
		double lastprice = 0;
		double price = 0;
		if (!PriceVector.empty()) {
			if (NumPricesToday == 96) {
				for (auto priceCount=0;priceCount<96;priceCount++) {
					price = PriceVector.at(priceCount);
					lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
				}
			} else if( NumPricesToday == 23) { // Starting DST
				for (auto priceCount=0;priceCount<23;priceCount++) {
					price = PriceVector.at(priceCount);
					if(priceCount<2) {
						lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
					} else if (priceCount == 2){
						lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
						lastprice = AddPrice(buff, priceCount+1, price, priceCount, lastprice);
					} else {
						lastprice = AddPrice(buff, priceCount+1, price, priceCount, lastprice);			
					}
				}
			} else {  // Ending DST
			} 
				
		}
		if (!PriceVectorTomorrow.empty()) {
			price = PriceVectorTomorrow.at(0);
			if (price > 0)
				lastprice = AddPrice(buff, 96, price, 95, lastprice);
		}
	}


	// Draw the graph tomorrow
	void DrawPriceGraphTomorrow (display::Display *buff) {
		double lastprice = 0;
		double price = 0;

		if (!PriceVectorTomorrow.empty()) {
		tomorrowsMaxPrice = *max_element(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end());
		tomorrowsMinPrice = *min_element(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end());
		tomorrowsAverage = accumulate(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end() ,0.0) / PriceVectorTomorrow.size();

		for (int priceCount=0;priceCount<96;priceCount++) {
			price = PriceVectorTomorrow.at(priceCount);
			lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
			}
		}
		if (PriceVectorTomorrow.empty()) {
			tomorrowsMaxPrice = 0;
			tomorrowsMinPrice = 0;
			tomorrowsAverage = 0;
		}

	}


	
	// Deserialize the JSON string from NordPool and put it into two vectors
	void SetPrices(std::string day) {
		std::string prices;

		if(day == "today") {
			prices = TodaysPrices;
			NumPricesToday = 0;

			if (prices.length() > 10) {
				prices.erase(0, 1);
				prices.replace(prices.size() - 1, 1, " ");

				PriceVector.clear();
				std::stringstream ss(prices);
				double i;
				while (ss >> i)
				{
					PriceVector.push_back(i);
					NumPricesToday += 1;
					if (ss.peek() == ',')
						ss.ignore();
				}
		//  Uncomment the lines below to get log messages to show the contents of the Todays vector
		//		for (auto it = PriceVector.begin(); it != PriceVector.end(); it++) { 
		//			ss << *it << " ";
		//		}
		//	    ESP_LOGD("TodaysPrices: ", ss.str().c_str());
			}

		}

		if(day == "tomorrow") {
			prices = TomorrowsPrices;
			NumPricesTomorrow = 0;
			
			if (prices == "" || prices == "[]" || prices == "None") {
				ESP_LOGD("PDSP","Clearing tomorrow prices");
				PriceVectorTomorrow.clear();
			}

   			if (prices.length() > 10) {
				prices.erase(0, 1);
				prices.replace(prices.size() - 1, 1, " ");

				PriceVectorTomorrow.clear();
				std::stringstream st(prices);
				double i;
				while (st >> i)
				{
					PriceVectorTomorrow.push_back(i);
					NumPricesTomorrow += 1;
					if (st.peek() == ',')
						st.ignore();
				}
		//  Uncomment the lines below to get log messages to show the contents of the Tomorrow vector
		//		for (auto it = PriceVectorTomorrow.begin(); it != PriceVectorTomorrow.end(); it++) { 
		//			st << *it << " ";
		//		}
		//	    ESP_LOGD("TomorrowsPrices: ", st.str().c_str());
			}
		}
	}
	
private:
	display::Display *vbuff;
	
	int graphWidth, graphHeight, xPos, yPos;
	float xFactor, yFactor, xFactorGrid;
	
	double prevDailyEnergy, accumulatedCost;
	int NumPricesToday,NumPricesTomorrow;

	std::vector<double> PriceVector;
	std::vector<double> PriceVectorTomorrow;

	void setPos(int x, int y) {xPos = x; yPos = y;}
	void setWidth (int width) {graphWidth = width;}
	void setHeigh(int height) {graphHeight = height;}
	
	void DrawGraphLine(display::Display *buff, double x1, double x2, double y1, double y2, Color color = COLOR_ON) {
		buff->line(xPos + x1*xFactor, yPos+graphHeight-(y1*yFactor), xPos + x2*xFactor, yPos+graphHeight-(y2*yFactor), color);		
		// buff->line(xPos + x1*xFactor, yPos+1+graphHeight-(y1*yFactor), xPos + x2*xFactor, yPos+graphHeight-(y2*yFactor), color);	// Make line a bit thicker	
//		buff->line(xPos + x1*xFactor, yPos+2+graphHeight-(y1*yFactor), xPos + x2*xFactor, yPos+graphHeight-(y2*yFactor), color);		
//		ESP_LOGD("GraphGrid x1: ", String(xPos + x1*xFactor).c_str());
	}

	double AddPrice(display::Display *buff, int hour, double price, int lastHour, double lastPrice)	{
		if(lastHour<0)
			lastHour=0;	  
		if(lastPrice==0)
			lastPrice = price;	
		DrawGraphLine(buff, lastHour, hour, lastPrice, price, PriceColour(lastPrice));
	  return price;
	}
	
	Color PriceColour (float nNewPrice) {
		Color colour;
		if (inRange(nNewPrice, id(EXTREMELY_EXPENSIVE).state, 100)) {colour = COLOR_CSS_MAROON;}
		else if (inRange(nNewPrice, id(VERY_EXPENSIVE).state, id(EXTREMELY_EXPENSIVE).state)){colour =  COLOR_CSS_RED;}
		else if (inRange(nNewPrice, id(EXPENSIVE).state, id(VERY_EXPENSIVE).state)){colour =  COLOR_CSS_ORANGE;}
		else if (inRange(nNewPrice, id(NORMAL).state, id(EXPENSIVE).state)){colour = COLOR_CSS_GREENYELLOW;}
		else if (inRange(nNewPrice, id(CHEAP).state, id(NORMAL).state)){colour = COLOR_CSS_GREEN;}
		else if (inRange(nNewPrice, id(VERY_CHEAP).state, id(CHEAP).state)){colour = COLOR_CSS_DARKGREEN;}
		else if (inRange(nNewPrice, -100, id(VERY_CHEAP).state)){colour = COLOR_CSS_DARKGREEN;}     
		return colour; 
	}
	
//	double CalculateAccumulatedCost(double currentPrice, double dailyEnergy) {
//		if (currentPrice == 0)
//			return 0;
//		double nEnergyDelta = dailyEnergy - prevDailyEnergy; 	
//		prevDailyEnergy = dailyEnergy;
//		accumulatedCost += (nEnergyDelta * currentPrice);
//		//SaveValueToNvm("AccumulatedCost", accumulatedCost);
//		return accumulatedCost;
//	}

	bool inRange(float val, float minimum, float maximum)
	{
	  return ((minimum <= val) && (val <= maximum));
	}
	



}; //class







