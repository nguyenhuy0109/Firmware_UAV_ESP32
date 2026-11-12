### Tài liệu tham khảo
>> Repo Github: https://github.com/bitcraze/Bitcraze_PMW3901.git
#### Xây dựng hàm:
>> private
1. uint8_t m_csPin;
2. bool m_cs_initialized = false;
3. esp_err_t init_CS_GPIO();                                       // hàm set chân
4. esp_err_t registerWrite(uint8_t reg, uint8_t value);            // hàm ghi 
5. esp_err_t registerRead(uint8_t reg, uint8_t *value);            // hàm đọc 
6. esp_err_t initRegister();                                       // hàm khởi tạo tối ưu
7. spi_device_handle_t m_dev = nullptr;                            // tham chiếu thiết bị đăng ký

>> public
8. PMW3901(uint8_t csPin);                                         // lưu chân cs
9.  bool init_SPI_PMW3901();                                        // check khởi tạo SPI
10. esp_err_t readMotionCount(int16_t *deltaX, int16_t *deltaY);   // đọc chuyển động, cả chiều âm, dương
11. bool begin();                                                  // hàm check khởi tạo

>> Lưu ý:
>> Chờ vài micro giây đến khoảng 100 µs: thường dùng esp_rom_delay_us() khi thiết bị yêu cầu khoảng chờ sát giữa các thao tác.
>> Từ khoảng 1 ms trở lên: thường ưu tiên vTaskDelay() để task nhường CPU. Nhưng nếu datasheet yêu cầu thời gian chính xác hoặc tối thiểu dưới millisecond thì phải xem độ phân giải timer/tick và cách chờ phù hợp.
>> Vùng vài trăm micro giây đến 1 ms: tùy yêu cầu thời gian và độ phân giải tick; đừng chọn chỉ dựa vào con số.

- Chờ micro giây thì esp_rom_delay_us (chiếm cpu, bus_wait), còn ms thì ưu tiên vTaskDelay (cpu có thể làm việc khác, block task)