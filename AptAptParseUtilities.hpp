#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <string_view>

namespace Apt::AptParseUtilities {
    struct UnparsedData {
        struct UnparsedDataView {
            UnparsedDataView(UnparsedData* source, const std::string_view view, std::size_t viewPosition) : 
                source{source}, view{view}, viewPosition{viewPosition}
            {
                if(std::string_view{this->source->data}.substr(this->viewPosition, this->view.size()) != this->view) {
                    throw std::invalid_argument{"Invalid source / view / viewPosition!"};
                }
            }

            UnparsedData* getFullData() noexcept {
                return this->source;
            }

            std::string_view readFront(const std::size_t length) {
                auto front = this->popPrefix(length);
                front.markAllAsRead();
                return front.view;
            }

            template<typename T>
            T readFrontAs() {
                static_assert(std::is_trivially_copyable_v<T>);
                auto front = this->readFront(sizeof(T));
                return *reinterpret_cast<const T*>(front.data());
            }

            template<typename T>
            UnparsedDataView& readFrontTo(T& destination) {
                destination = this->readFrontAs<T>();
                return *this;
            }
            
            UnparsedDataView subView(const std::size_t from) const {
                return this->split(from).second;
            }

            std::size_t absolutePosition() const noexcept {
                return this->viewPosition;
            }

        private:

            void markAsRead(const std::size_t begin, const std::size_t end) {
                this->source->updateUnparsed(begin + viewPosition, end + viewPosition);
            }

            void markAllAsRead() {
                this->markAsRead(0, this->view.size());
            }

            std::pair<UnparsedDataView, UnparsedDataView> split(const std::size_t position) const {
                if(position > this->view.size()) {
                    throw std::out_of_range{"position > this->view.size() when splitting UnparsedDataView"};
                }
                const auto first = UnparsedDataView(this->source, this->view.substr(0, position), this->viewPosition);
                const auto second = UnparsedDataView(this->source, this->view.substr(position), this->viewPosition + position);
                return {first, second};
            }

            UnparsedDataView popPrefix(const std::size_t length) {
                const auto [prefix, remained] = this->split(length);
                *this = remained;
                return prefix;
            }

            UnparsedData* source;
            std::string_view view;
            std::size_t viewPosition;
        };

        UnparsedData() : UnparsedData{std::string_view{}} {}

        template<typename StringType>
        UnparsedData(StringType&& data) : data{std::forward<StringType>(data)} {
            unparsedBeginEnd = { { 0, this->data.size() } };
        }

        template<typename StringType>
        void reset(StringType&& data) {
            *this = UnparsedData{ std::forward<StringType>(data) };
        }

        UnparsedDataView getView() {
            return UnparsedDataView{this, this->data, 0};
        }

        void updateUnparsed(const std::size_t beginParsed, const std::size_t endParsed) {
            if(beginParsed >= endParsed) {
                throw std::invalid_argument{"beginParsed >= endParsed"};
            }

            if(unparsedBeginEnd.upper_bound(beginParsed) == unparsedBeginEnd.begin()) {
                throw std::logic_error{ "Should never happen!" };
            }
            const auto first = std::prev(unparsedBeginEnd.upper_bound(beginParsed));
            const auto last = unparsedBeginEnd.lower_bound(endParsed);
            auto toBeRemoved = std::vector<std::size_t>{};
            auto toBeAdded = std::vector<std::pair<std::size_t, std::size_t>>{};
            std::for_each(first, last, [&](const auto& pair) {
                const auto [begin, end] = pair;
                if (end <= beginParsed || begin >= endParsed) {
                    return;
                }

                toBeRemoved.emplace_back(begin);

                if (begin < beginParsed) {
                    toBeAdded.emplace_back(begin, beginParsed);
                }

                if (endParsed < end) {
                    toBeAdded.emplace_back(endParsed, end);
                }
            });

            for (const auto key : toBeRemoved) {
                unparsedBeginEnd.erase(key);
            }

            for (const auto& [begin, end] : toBeAdded) {
                unparsedBeginEnd.emplace(begin, end);
            }
        }

        std::string data;
        std::map<std::size_t, std::size_t> unparsedBeginEnd;
    };
}