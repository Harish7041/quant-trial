#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <string_view>
#include <charconv>
#include <cstdint>

// Represents a single price level in the order book.
// Tracks total volume and number of orders.
struct LevelInfo {
    int64_t total_size{0};
    int32_t order_count{0};
};

// Stores essential info about an order for O(1) lookup.
struct OrderInfo {
    int64_t price;
    char side;
};

// Custom parser for performance. Avoids std::stringstream overhead.
// Parses a substring and converts it to the specified type.
template<typename T>
T parse_field(std::string_view& line_sv) {
    auto pos = line_sv.find(',');
    std::string_view field = line_sv.substr(0, pos);
    if (pos != std::string_view::npos) {
        line_sv.remove_prefix(pos + 1);
    } else {
        line_sv.remove_prefix(line_sv.size());
    }

    T value{};
    // Use std::from_chars for fast, locale-independent conversion.
    if constexpr (std::is_integral_v<T>) {
        std::from_chars(field.data(), field.data() + field.size(), value);
    } else { // Specifically for the price double
        std::from_chars(field.data(), field.data() + field.size(), value);
    }
    return value;
}

// Skips a specified number of fields in the string_view.
void skip_fields(std::string_view& line_sv, int count) {
    for (int i = 0; i < count; ++i) {
        auto pos = line_sv.find(',');
        if (pos != std::string_view::npos) {
            line_sv.remove_prefix(pos + 1);
        } else {
            line_sv.remove_prefix(line_sv.size());
            break;
        }
    }
}

// Function to write the current top-10 levels of the order book to the output file.
void write_mbp_output(std::ofstream& outputFile,
                      long ts_event,
                      const std::map<int64_t, LevelInfo, std::greater<int64_t>>& bids,
                      const std::map<int64_t, LevelInfo>& asks) {

    outputFile << ts_event;

    auto bid_it = bids.begin();
    auto ask_it = asks.begin();

    for (int i = 0; i < 10; ++i) {
        // Ask Price, Size, Count
        if (ask_it != asks.end()) {
            outputFile << ',' << ask_it->first << ',' << ask_it->second.total_size << ',' << ask_it->second.order_count;
            ++ask_it;
        } else {
            outputFile << ",,,";
        }
        // Bid Price, Size, Count
        if (bid_it != bids.end()) {
            outputFile << ',' << bid_it->first << ',' << bid_it->second.total_size << ',' << bid_it->second.order_count;
            ++bid_it;
        } else {
            outputFile << ",,,";
        }
    }
    outputFile << '\n';
}

int main(int argc, char* argv[]) {
    // Fast I/O settings
    std::ios_base::sync_with_stdio(false);

    if (argc != 2) {
        std::cerr << "Usage: ./reconstruction <mbo_file.csv>\n";
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening input file: " << argv[1] << "\n";
        return 1;
    }

    std::ofstream outputFile("mbp.csv");
    // Write header exactly as specified in the sample output
    outputFile << "ts_event,ask_px_00,ask_sz_00,ask_ct_00,bid_px_00,bid_sz_00,bid_ct_00,ask_px_01,ask_sz_01,ask_ct_01,bid_px_01,bid_sz_01,bid_ct_01,ask_px_02,ask_sz_02,ask_ct_02,bid_px_02,bid_sz_02,bid_ct_02,ask_px_03,ask_sz_03,ask_ct_03,bid_px_03,bid_sz_03,bid_ct_03,ask_px_04,ask_sz_04,ask_ct_04,bid_px_04,bid_sz_04,bid_ct_04,ask_px_05,ask_sz_05,ask_ct_05,bid_px_05,bid_sz_05,bid_ct_05,ask_px_06,ask_sz_06,ask_ct_06,bid_px_06,bid_sz_06,bid_ct_06,ask_px_07,ask_sz_07,ask_ct_07,bid_px_07,bid_sz_07,bid_ct_07,ask_px_08,ask_sz_08,ask_ct_08,bid_px_08,bid_sz_08,bid_ct_08,ask_px_09,ask_sz_09,ask_ct_09,bid_px_09,bid_sz_09,bid_ct_09\n";


    // Order book data structures
    std::map<int64_t, LevelInfo, std::greater<int64_t>> bids;
    std::map<int64_t, LevelInfo> asks;
    std::unordered_map<uint64_t, OrderInfo> order_map;

    std::string line;
    // Rule 1: Ignore header and initial 'R' row (clear book action)
    std::getline(inputFile, line);
    std::getline(inputFile, line);

    while (std::getline(inputFile, line)) {
        std::string_view line_sv(line);

        // --- Fast, Optimized Parsing ---
        skip_fields(line_sv, 1); // ts_recv
        long ts_event = parse_field<long>(line_sv);
        skip_fields(line_sv, 3); // rtype, publisher_id, instrument_id

        char action = line_sv[0];
        skip_fields(line_sv, 1);

        char side = line_sv[0];
        skip_fields(line_sv, 1);

        double price_double = parse_field<double>(line_sv);
        // Convert price to integer to avoid floating point issues.
        // Scaling by 10000 ensures precision for prices like X.1234.
        int64_t price = static_cast<int64_t>(price_double * 10000.0);
        int64_t size = parse_field<int64_t>(line_sv);
        skip_fields(line_sv, 1); // channel_id
        uint64_t order_id = parse_field<uint64_t>(line_sv);
        // --- End Parsing ---

        // Main logic based on action type
        switch (action) {
            case 'A': { // ADD
                if (side == 'B') {
                    bids[price].total_size += size;
                    bids[price].order_count++;
                } else if (side == 'A') {
                    asks[price].total_size += size;
                    asks[price].order_count++;
                }
                order_map[order_id] = {price, side};
                break;
            }
            case 'C': { // CANCEL
                auto it = order_map.find(order_id);
                if (it != order_map.end()) {
                    OrderInfo info = it->second;
                    if (info.side == 'B') {
                        bids[info.price].total_size -= size;
                        bids[info.price].order_count--;
                        if (bids[info.price].total_size <= 0) {
                            bids.erase(info.price);
                        }
                    } else if (info.side == 'A') {
                        asks[info.price].total_size -= size;
                        asks[info.price].order_count--;
                        if (asks[info.price].total_size <= 0) {
                            asks.erase(info.price);
                        }
                    }
                    order_map.erase(it);
                }
                break; // <-- FIX: This break was missing.
            }
            case 'T': { // TRADE (Special Logic)
                // Rule 3: Ignore if side is 'N'
                if (side == 'N') {
                    break;
                }
                // Rule 2: Trade affects the OPPOSITE side of the book.
                if (side == 'A') { // Aggressive Ask (sell) hits a resting Bid
                    if (bids.count(price)) {
                        bids[price].total_size -= size;
                        // A trade implies a resting order was filled. We assume it's one order.
                        bids[price].order_count--;
                        if (bids[price].total_size <= 0) {
                            bids.erase(price);
                        }
                    }
                } else if (side == 'B') { // Aggressive Bid (buy) hits a resting Ask
                    if (asks.count(price)) {
                        asks[price].total_size -= size;
                        asks[price].order_count--;
                        if (asks[price].total_size <= 0) {
                            asks.erase(price);
                        }
                    }
                }
                break;
            }
            case 'F': // FILL - Ignored per instructions, as it's part of the Trade sequence.
                continue; // IMPORTANT: 'continue' skips the MBP output for this line
            default:
                break;
        }
        // Generate and write MBP-10 output for the current state
        write_mbp_output(outputFile, ts_event, bids, asks);
    }

    return 0;
}
