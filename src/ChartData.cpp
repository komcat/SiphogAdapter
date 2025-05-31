#include "ChartData.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <mutex>      // Add this for std::mutex and std::lock_guard
#include <atomic>     // Add this for std::atomic

namespace SiphogLib {

    ChartDataManager::ChartDataManager(size_t maxDataPoints)
        : maxPoints(maxDataPoints), currentIndex(0), bufferFull(false) {
        std::lock_guard<std::mutex> lock(dataMutex);

        // Reserve space for better performance with vectors
        timeData.reserve(maxDataPoints);
        sledCurrentData.reserve(maxDataPoints);
        sledTempData.reserve(maxDataPoints);
        tecCurrentData.reserve(maxDataPoints);
        photoCurrentData.reserve(maxDataPoints);
        sagPowerData.reserve(maxDataPoints);
        sldPowerData.reserve(maxDataPoints);
        caseTempData.reserve(maxDataPoints);
        opAmpTempData.reserve(maxDataPoints);
        supplyVoltageData.reserve(maxDataPoints);
        adcCountIData.reserve(maxDataPoints);
        adcCountQData.reserve(maxDataPoints);
    }

    void ChartDataManager::addDataPoint(const SiphogMessageModel& message) {
        std::lock_guard<std::mutex> lock(dataMutex);

        double currentTime = message.timeSeconds;

        // Calculate derived values
        double photoCurrentUa = message.sldPowerUw / 0.8 * 1e6; // Using PWR_MON_TRANSFER_FUNC

        size_t maxPts = maxPoints.load();

        if (timeData.size() < maxPts) {
            // Still filling the buffer
            timeData.push_back(currentTime);
            sledCurrentData.push_back(message.sledCurrent);
            sledTempData.push_back(message.sledTemp);
            tecCurrentData.push_back(message.tecCurrent);
            photoCurrentData.push_back(photoCurrentUa);
            sagPowerData.push_back(message.sagPowerV);
            sldPowerData.push_back(message.sldPowerUw);
            caseTempData.push_back(message.caseTemp);
            opAmpTempData.push_back(message.opAmpTemp);
            supplyVoltageData.push_back(message.supplyVoltage);
            adcCountIData.push_back(message.adcCountI);
            adcCountQData.push_back(message.adcCountQ);
        }
        else {
            // Buffer is full, use circular buffer approach
            bufferFull.store(true);
            size_t idx = currentIndex.load() % maxPts;

            // Ensure vectors are the right size
            if (timeData.size() != maxPts) {
                timeData.resize(maxPts);
                sledCurrentData.resize(maxPts);
                sledTempData.resize(maxPts);
                tecCurrentData.resize(maxPts);
                photoCurrentData.resize(maxPts);
                sagPowerData.resize(maxPts);
                sldPowerData.resize(maxPts);
                caseTempData.resize(maxPts);
                opAmpTempData.resize(maxPts);
                supplyVoltageData.resize(maxPts);
                adcCountIData.resize(maxPts);
                adcCountQData.resize(maxPts);
            }

            timeData[idx] = currentTime;
            sledCurrentData[idx] = message.sledCurrent;
            sledTempData[idx] = message.sledTemp;
            tecCurrentData[idx] = message.tecCurrent;
            photoCurrentData[idx] = photoCurrentUa;
            sagPowerData[idx] = message.sagPowerV;
            sldPowerData[idx] = message.sldPowerUw;
            caseTempData[idx] = message.caseTemp;
            opAmpTempData[idx] = message.opAmpTemp;
            supplyVoltageData[idx] = message.supplyVoltage;
            adcCountIData[idx] = message.adcCountI;
            adcCountQData[idx] = message.adcCountQ;

            currentIndex.store(idx + 1);
        }
    }

    void ChartDataManager::clear() {
        std::lock_guard<std::mutex> lock(dataMutex);

        timeData.clear();
        sledCurrentData.clear();
        sledTempData.clear();
        tecCurrentData.clear();
        photoCurrentData.clear();
        sagPowerData.clear();
        sldPowerData.clear();
        caseTempData.clear();
        opAmpTempData.clear();
        supplyVoltageData.clear();
        adcCountIData.clear();
        adcCountQData.clear();

        currentIndex.store(0);
        bufferFull.store(false);
    }

    size_t ChartDataManager::getDataPointCount() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return timeData.size();
    }

    bool ChartDataManager::hasData() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return !timeData.empty();
    }

    std::vector<double> ChartDataManager::getTimeVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (!bufferFull.load()) {
            // Simple case: return copy of data in order
            return std::vector<double>(timeData.begin(), timeData.end());
        }
        else {
            // Circular buffer: reconstruct chronological order
            std::vector<double> result;
            size_t maxPts = maxPoints.load();
            size_t currIdx = currentIndex.load() % maxPts;

            result.reserve(maxPts);

            // Add from currentIndex to end
            for (size_t i = currIdx; i < timeData.size(); ++i) {
                result.push_back(timeData[i]);
            }
            // Add from beginning to currentIndex
            for (size_t i = 0; i < currIdx; ++i) {
                result.push_back(timeData[i]);
            }

            return result;
        }
    }

    std::vector<double> ChartDataManager::getSagPowerVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (!bufferFull.load()) {
            return std::vector<double>(sagPowerData.begin(), sagPowerData.end());
        }
        else {
            std::vector<double> result;
            size_t maxPts = maxPoints.load();
            size_t currIdx = currentIndex.load() % maxPts;

            result.reserve(maxPts);

            for (size_t i = currIdx; i < sagPowerData.size(); ++i) {
                result.push_back(sagPowerData[i]);
            }
            for (size_t i = 0; i < currIdx; ++i) {
                result.push_back(sagPowerData[i]);
            }

            return result;
        }
    }

    std::vector<double> ChartDataManager::getSldPowerVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (!bufferFull.load()) {
            return std::vector<double>(sldPowerData.begin(), sldPowerData.end());
        }
        else {
            std::vector<double> result;
            size_t maxPts = maxPoints.load();
            size_t currIdx = currentIndex.load() % maxPts;

            result.reserve(maxPts);

            for (size_t i = currIdx; i < sldPowerData.size(); ++i) {
                result.push_back(sldPowerData[i]);
            }
            for (size_t i = 0; i < currIdx; ++i) {
                result.push_back(sldPowerData[i]);
            }

            return result;
        }
    }

    std::vector<double> ChartDataManager::getSledCurrentVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(sledCurrentData.begin(), sledCurrentData.end());
    }

    std::vector<double> ChartDataManager::getSledTempVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(sledTempData.begin(), sledTempData.end());
    }

    std::vector<double> ChartDataManager::getTecCurrentVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(tecCurrentData.begin(), tecCurrentData.end());
    }

    std::vector<double> ChartDataManager::getPhotoCurrentVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(photoCurrentData.begin(), photoCurrentData.end());
    }

    std::vector<double> ChartDataManager::getCaseTempVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(caseTempData.begin(), caseTempData.end());
    }

    std::vector<double> ChartDataManager::getOpAmpTempVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(opAmpTempData.begin(), opAmpTempData.end());
    }

    std::vector<double> ChartDataManager::getSupplyVoltageVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(supplyVoltageData.begin(), supplyVoltageData.end());
    }

    std::vector<double> ChartDataManager::getAdcCountIVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(adcCountIData.begin(), adcCountIData.end());
    }

    std::vector<double> ChartDataManager::getAdcCountQVector() const {
        std::lock_guard<std::mutex> lock(dataMutex);
        return std::vector<double>(adcCountQData.begin(), adcCountQData.end());
    }

    double ChartDataManager::getLatestTime() const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (timeData.empty()) return 0.0;

        if (!bufferFull.load()) {
            return timeData.back();
        }
        else {
            size_t maxPts = maxPoints.load();
            size_t latestIndex = (currentIndex.load() == 0) ? maxPts - 1 : (currentIndex.load() - 1) % maxPts;
            return timeData[latestIndex];
        }
    }

    std::pair<double, double> ChartDataManager::getTimeWindow(double windowSeconds) const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (timeData.empty()) {
            return { 0.0, windowSeconds };
        }

        double latestTime = 0.0;
        if (!bufferFull.load()) {
            latestTime = timeData.back();
        }
        else {
            size_t maxPts = maxPoints.load();
            size_t latestIndex = (currentIndex.load() == 0) ? maxPts - 1 : (currentIndex.load() - 1) % maxPts;
            latestTime = timeData[latestIndex];
        }

        double startTime = latestTime - windowSeconds;
        return { startTime, latestTime };
    }

    void ChartDataManager::setMaxPoints(size_t maxDataPoints) {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (maxDataPoints == maxPoints.load()) return;

        maxPoints.store(maxDataPoints);

        // If reducing size, trim data
        if (timeData.size() > maxDataPoints) {
            size_t elementsToRemove = timeData.size() - maxDataPoints;

            timeData.erase(timeData.begin(), timeData.begin() + elementsToRemove);
            sledCurrentData.erase(sledCurrentData.begin(), sledCurrentData.begin() + elementsToRemove);
            sledTempData.erase(sledTempData.begin(), sledTempData.begin() + elementsToRemove);
            tecCurrentData.erase(tecCurrentData.begin(), tecCurrentData.begin() + elementsToRemove);
            photoCurrentData.erase(photoCurrentData.begin(), photoCurrentData.begin() + elementsToRemove);
            sagPowerData.erase(sagPowerData.begin(), sagPowerData.begin() + elementsToRemove);
            sldPowerData.erase(sldPowerData.begin(), sldPowerData.begin() + elementsToRemove);
            caseTempData.erase(caseTempData.begin(), caseTempData.begin() + elementsToRemove);
            opAmpTempData.erase(opAmpTempData.begin(), opAmpTempData.begin() + elementsToRemove);
            supplyVoltageData.erase(supplyVoltageData.begin(), supplyVoltageData.begin() + elementsToRemove);
            adcCountIData.erase(adcCountIData.begin(), adcCountIData.begin() + elementsToRemove);
            adcCountQData.erase(adcCountQData.begin(), adcCountQData.begin() + elementsToRemove);

            currentIndex.store(0);
            bufferFull.store(false);
        }

        // Reserve new space
        timeData.reserve(maxDataPoints);
        sledCurrentData.reserve(maxDataPoints);
        sledTempData.reserve(maxDataPoints);
        tecCurrentData.reserve(maxDataPoints);
        photoCurrentData.reserve(maxDataPoints);
        sagPowerData.reserve(maxDataPoints);
        sldPowerData.reserve(maxDataPoints);
        caseTempData.reserve(maxDataPoints);
        opAmpTempData.reserve(maxDataPoints);
        supplyVoltageData.reserve(maxDataPoints);
        adcCountIData.reserve(maxDataPoints);
        adcCountQData.reserve(maxDataPoints);
    }

    ChartDataManager::DataStats ChartDataManager::getDataStats(const std::vector<double>& data) const {
        DataStats stats;

        if (data.empty()) {
            return stats;
        }

        stats.latest = data.back();
        stats.min = *std::min_element(data.begin(), data.end());
        stats.max = *std::max_element(data.begin(), data.end());
        stats.mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();

        return stats;
    }

    bool ChartDataManager::exportToCsv(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (timeData.empty()) {
            return false;
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        // Write header
        file << "Time,SLED_Current,SLED_Temp,TEC_Current,Photo_Current,SAG_Power,SLD_Power,";
        file << "Case_Temp,OpAmp_Temp,Supply_Voltage,ADC_Count_I,ADC_Count_Q\n";

        // Write data in chronological order
        file << std::fixed << std::setprecision(6);

        if (!bufferFull.load()) {
            // Simple case: data is in order
            for (size_t i = 0; i < timeData.size(); ++i) {
                file << timeData[i] << ","
                    << sledCurrentData[i] << ","
                    << sledTempData[i] << ","
                    << tecCurrentData[i] << ","
                    << photoCurrentData[i] << ","
                    << sagPowerData[i] << ","
                    << sldPowerData[i] << ","
                    << caseTempData[i] << ","
                    << opAmpTempData[i] << ","
                    << supplyVoltageData[i] << ","
                    << adcCountIData[i] << ","
                    << adcCountQData[i] << "\n";
            }
        }
        else {
            // Circular buffer: write in chronological order
            size_t maxPts = maxPoints.load();
            size_t currIdx = currentIndex.load() % maxPts;

            // Write from currentIndex to end
            for (size_t i = currIdx; i < timeData.size(); ++i) {
                file << timeData[i] << ","
                    << sledCurrentData[i] << ","
                    << sledTempData[i] << ","
                    << tecCurrentData[i] << ","
                    << photoCurrentData[i] << ","
                    << sagPowerData[i] << ","
                    << sldPowerData[i] << ","
                    << caseTempData[i] << ","
                    << opAmpTempData[i] << ","
                    << supplyVoltageData[i] << ","
                    << adcCountIData[i] << ","
                    << adcCountQData[i] << "\n";
            }
            // Write from beginning to currentIndex
            for (size_t i = 0; i < currIdx; ++i) {
                file << timeData[i] << ","
                    << sledCurrentData[i] << ","
                    << sledTempData[i] << ","
                    << tecCurrentData[i] << ","
                    << photoCurrentData[i] << ","
                    << sagPowerData[i] << ","
                    << sldPowerData[i] << ","
                    << caseTempData[i] << ","
                    << opAmpTempData[i] << ","
                    << supplyVoltageData[i] << ","
                    << adcCountIData[i] << ","
                    << adcCountQData[i] << "\n";
            }
        }

        return true;
    }

    void ChartDataManager::trimToMaxPointsUnsafe() {
        // This method should only be called when mutex is already locked
        // Implementation moved to setMaxPoints method
    }

} // namespace SiphogLib