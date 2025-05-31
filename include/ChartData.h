#pragma once
#include <vector>
#include <string>
#include <mutex>      // Add this for std::mutex
#include <atomic>     // Add this for std::atomic
#include "SiphogMessageModel.h"

namespace SiphogLib {

    /**
     * @brief Thread-safe chart data manager for SiPhOG measurements
     */
    class ChartDataManager {
    private:
        static const size_t DEFAULT_MAX_POINTS = 1000;

        mutable std::mutex dataMutex; // Protect all data access

        std::vector<double> timeData;
        std::vector<double> sledCurrentData;
        std::vector<double> sledTempData;
        std::vector<double> tecCurrentData;
        std::vector<double> photoCurrentData;
        std::vector<double> sagPowerData;
        std::vector<double> sldPowerData;
        std::vector<double> caseTempData;
        std::vector<double> opAmpTempData;
        std::vector<double> supplyVoltageData;
        std::vector<double> adcCountIData;
        std::vector<double> adcCountQData;

        std::atomic<size_t> maxPoints;
        std::atomic<size_t> currentIndex;
        std::atomic<bool> bufferFull;

        void trimToMaxPointsUnsafe(); // Call only when mutex is locked

    public:
        /**
         * @brief Constructor
         * @param maxDataPoints Maximum number of data points to keep in memory
         */
        explicit ChartDataManager(size_t maxDataPoints = DEFAULT_MAX_POINTS);

        /**
         * @brief Add a new data point from SiPhOG message (thread-safe)
         * @param message SiPhOG message containing sensor data
         */
        void addDataPoint(const SiphogMessageModel& message);

        /**
         * @brief Clear all chart data (thread-safe)
         */
        void clear();

        /**
         * @brief Get the number of data points stored (thread-safe)
         * @return Number of data points
         */
        size_t getDataPointCount() const;

        /**
         * @brief Check if chart has data (thread-safe)
         * @return true if data is available, false otherwise
         */
        bool hasData() const;

        /**
         * @brief Get time data as vector for plotting (thread-safe copy)
         * @return Vector of time values
         */
        std::vector<double> getTimeVector() const;

        /**
         * @brief Get SLED current data as vector for plotting (thread-safe copy)
         * @return Vector of SLED current values (mA)
         */
        std::vector<double> getSledCurrentVector() const;

        /**
         * @brief Get SLED temperature data as vector for plotting (thread-safe copy)
         * @return Vector of SLED temperature values (°C)
         */
        std::vector<double> getSledTempVector() const;

        /**
         * @brief Get TEC current data as vector for plotting (thread-safe copy)
         * @return Vector of TEC current values (mA)
         */
        std::vector<double> getTecCurrentVector() const;

        /**
         * @brief Get photo current data as vector for plotting (thread-safe copy)
         * @return Vector of photo current values (µA)
         */
        std::vector<double> getPhotoCurrentVector() const;

        /**
         * @brief Get Sagnac power data as vector for plotting (thread-safe copy)
         * @return Vector of Sagnac power values (V)
         */
        std::vector<double> getSagPowerVector() const;

        /**
         * @brief Get SLD power data as vector for plotting (thread-safe copy)
         * @return Vector of SLD power values (µW)
         */
        std::vector<double> getSldPowerVector() const;

        /**
         * @brief Get case temperature data as vector for plotting (thread-safe copy)
         * @return Vector of case temperature values (°C)
         */
        std::vector<double> getCaseTempVector() const;

        /**
         * @brief Get op amp temperature data as vector for plotting (thread-safe copy)
         * @return Vector of op amp temperature values (°C)
         */
        std::vector<double> getOpAmpTempVector() const;

        /**
         * @brief Get supply voltage data as vector for plotting (thread-safe copy)
         * @return Vector of supply voltage values (V)
         */
        std::vector<double> getSupplyVoltageVector() const;

        /**
         * @brief Get ADC I count data as vector for plotting (thread-safe copy)
         * @return Vector of ADC I count values (V)
         */
        std::vector<double> getAdcCountIVector() const;

        /**
         * @brief Get ADC Q count data as vector for plotting (thread-safe copy)
         * @return Vector of ADC Q count values (V)
         */
        std::vector<double> getAdcCountQVector() const;

        /**
         * @brief Get the latest time value (thread-safe)
         * @return Latest time value, or 0 if no data
         */
        double getLatestTime() const;

        /**
         * @brief Get time window for auto-scaling charts (thread-safe)
         * @param windowSeconds Time window in seconds
         * @return Pair of (min_time, max_time)
         */
        std::pair<double, double> getTimeWindow(double windowSeconds = 30.0) const;

        /**
         * @brief Set maximum number of data points to keep (thread-safe)
         * @param maxDataPoints New maximum
         */
        void setMaxPoints(size_t maxDataPoints);

        /**
         * @brief Get current maximum data points setting (thread-safe)
         * @return Maximum data points
         */
        size_t getMaxPoints() const { return maxPoints.load(); }

        /**
         * @brief Get statistics for a data series
         * @param data Data vector
         * @return Struct containing min, max, mean values
         */
        struct DataStats {
            double min = 0.0;
            double max = 0.0;
            double mean = 0.0;
            double latest = 0.0;
        };

        DataStats getDataStats(const std::vector<double>& data) const;

        /**
         * @brief Export chart data to CSV format (thread-safe)
         * @param filename Output filename
         * @return true if export successful, false otherwise
         */
        bool exportToCsv(const std::string& filename) const;
    };

} // namespace SiphogLib