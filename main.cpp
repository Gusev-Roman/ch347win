// TestCot.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <tuple>
#include <ranges>
#include <concepts>
#include <stdint.h>

#include <windows.h>
#include "inc/CH347DLL_EN.H"

namespace ra = std::ranges;
//namespace vi = ra::views;

using namespace std::literals;

#define W25X_ReadData            0x03
#define W25X_FastReadData        0x0B
#define W25X_ManufactDeviceID    0x90
#define W25X_JedecDeviceID       0x9F

// Let's simplify meta-programming with popular use-cases

// template allows you to write one TEMPLATE (in eng it means a pattern or guide
// to be used as model to produce something...) if I had a Model class which has
// a field of X type and If I wanted this class for each {int, float, std::string} 
// then without meta-programming:
/*
struct ModelInt {
    int value;
};

struct ModelFloat {
    float value;
};

struct ModelString {
    std::string value;
};
*/
// I would MANUALLY code each. meta-programming is a method asks compiler to do it
// instead of you.

template <typename T>
struct Model {
    T value;
};
// whenever you use it with a new type it will generate a new type for you.
// int => Model<int>
// float => Model<float>
// string => Model<std::string>
// they use same TEMPLATE but they are different. Model<int> != Model<float>
// Model<std::string> => <T> now part of the className

// virtual methods in template classes
/*
template <typename T>
struct Base {
    virtual void foo() = 0;
    virtual void bar() { std::cout << "base::bar\n"; }
};

template <typename T>
struct Derived : Base<T> {
    void foo() override { std::cout << "derived::foo\n"; }
};

void test_virtual_methods() {
    std::unique_ptr<Base<int>> ptr = std::make_unique<Derived<int>>();
    ptr->foo();
    ptr->bar();
}

struct MyType {
    constexpr operator std::string_view() const { return "MyType"sv; }
};

// template member methods
struct Wrapper {
    template <typename T>
        requires std::integral<T>
    constexpr auto add(T l, T r) const {
        return l + r;
    }

    template <typename T>
        requires std::convertible_to<std::remove_cvref_t<T>, std::string_view>
    constexpr auto foo(T&& arg) const {
        return std::string_view{ arg };
    }
};

void test_template_members() {
    constexpr Wrapper w{};
    static_assert(w.add(1, 2) == 3);
    static_assert(w.foo(MyType{}) == "MyType"sv);
}

// pass template Cont to class template
template <template <typename> class Cont, typename DT = int>
struct ContUser {
    Cont<DT> full_generic_cont;
    Cont<int> special_cont; // this time you don't need to pass DT too
};

void test_template_cont() {
    ContUser<std::vector, float> full;
    ContUser<std::vector> special;
}

// pass template Cont to function template
template <typename Container>
void template_cont_func(Container const& cont) { // universal (forward-ref) doesn't apply here
    using DT = typename Container::value_type;
    std::cout << typeid(cont).name() << " " << typeid(DT).name() << "\n";
}

void test_template_cont_func() {
    std::vector<int> v;
    template_cont_func(v);
}

// template static functions - think them like non-member function in .cpp files
struct Factory {
    template <typename T>
    static constexpr T create() {
        if constexpr (std::integral<T>) {
            return 0;
        }
        else if constexpr (std::floating_point<T>) {
            return 0.0;
        }
        else if constexpr (std::is_constructible_v<T, const char*>) {
            return T("default");
        }
        else {
            static_assert(sizeof(T) == 0, "Unsupported type");
        }
    }
};

void test_template_static_methods() {
    static_assert(Factory::create<int>() == 0);
    static_assert(Factory::create<float>() == 0.0);
}

// variadic template params of template classes
template <typename... Ts>
    requires(sizeof...(Ts) >= 1)
struct MostDerived : Ts... {};

template <typename... Ts>
    requires(sizeof...(Ts) >= 1)
struct MostBorrower : Ts... {
    using Ts::foo...;
};

template <typename... Ts>
    requires(sizeof...(Ts) >= 1)
struct DWrapper {
    std::tuple<Ts...> mdata;
};

template <typename T>
concept HasFoo = requires(T t) {
    { t.foo() };
};

template <typename... Ts>
    requires(HasFoo<Ts> && ...)
struct BorrowAllFoos : Ts... {
    using Ts::foo...;
};

// variadic template params of template methods (static and non-static)
struct Outer {
    template <typename... Ts>
    constexpr auto collect(Ts&&... args) {
        return std::tuple<Ts...>{args...};
    }
};

// constraints & concepts: controlling the T (type)
// there are multiple ways to use requires { type based + instance based + ... }
template <typename T>
    requires(std::integral<T>)
constexpr auto add(T x, T y) {
    return x + y;
}

// instance
template <typename It>
constexpr void next(It& it)
    requires requires {
    it++;     // simple ++ existence check
    it.foo(); // simple foo existence check
    { it.bar() } -> std::same_as<void>; // simple bar existence with return-type check
}
{
    it++;
}

// if constexpr => compile time conditional compilation via type-info
struct CExpr {
    template <typename T>
    constexpr void foo(T arg) const {
        if constexpr (std::integral<std::remove_cvref_t<T>>) {
            std::cout << "functions easily captures the types in their headers\n";
        }
        else {
            std::cout << "it's not integral\n";
        }
    }

    template <typename T>
    static constexpr void bar(T arg) {
        if constexpr (std::integral<std::remove_cvref_t<T>>) {
            std::cout << "functions easily captures the types in their headers\n";
        }
        else {
            std::cout << "it's not integral\n";
        }
    }
};

void test_compile_time_conditional() {
    CExpr exp0;
    constexpr CExpr exp1;
    exp0.foo(0);
    exp1.foo(0);
    exp0.foo("hakan");
    exp1.foo("gedek");
    CExpr::bar(0);
    CExpr::bar("hakan");
}

// universal ctor: wrap the ctor with another template variable + && =>
// convertible or same_as or... just be careful.
template <typename T>
struct Sample {
    template <typename U>
    Sample(U&& value) : m_value(std::forward<U>(value)) {}

    template <typename U>
    constexpr auto update(U&& value) {
        m_value = std::forward<U>(value); // triggers correct assignment op
    }

    T m_value;
};

void test_universal_ctor() {
    Sample<int> s{ 0 };
    int x = 0;
    Sample<int> s1{ x };
    const int y = 0;
    Sample<int> s2{ y };
}
*/
UINT16 SPI_Flash_ReadID(ULONG port)
{
    UINT16 Temp = 0;
    UCHAR   cmd = W25X_ManufactDeviceID;
    UCHAR   zero = 0;
    UCHAR   data = 0xFF;

    //GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
    CH347SPI_ChangeCS(port, 0);            // active!
    //SPI1_ReadWriteByte(W25X_ManufactDeviceID);
    CH347SPI_WriteRead(port, 0, 1, &cmd); // auto cs if off
    //SPI1_ReadWriteByte(0x00);
    CH347SPI_Write(port, 0, 1, 1, &zero); // WriteStep <= Length
    CH347SPI_Write(port, 0, 1, 1, &zero);
    CH347SPI_Write(port, 0, 1, 1, &zero);

    //Temp |= SPI1_ReadWriteByte(0xFF) << 8;
    CH347SPI_WriteRead(port, 0, 1, &data);  // data едет в порт и в нее же загружается результат
    Temp |= data << 8;
    //Temp |= SPI1_ReadWriteByte(0xFF);
    data = 0xFF;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data;
    //GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
    CH347SPI_ChangeCS(port, 1);            // INactive!

    return Temp;
}
/* port не несет смысловой нагрузки в данной версии библиотеки, работает значение 0 */
UINT16 SPI_Flash_ReadJEDEC(ULONG port) {
    UINT16 Temp = 0;
    UCHAR cmd = W25X_JedecDeviceID;
    UCHAR zero = 0;
    UCHAR data = 0;

    CH347SPI_ChangeCS(port, 0);            // active!
    CH347SPI_WriteRead(port, 0, 1, &cmd);
    CH347SPI_WriteRead(port, 0, 1, &data);  // 0xEF
    printf("pre-jedec is %x\r\n", data);
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data << 8;
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data;
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    printf("post-jedec is %x\r\n", data);
    CH347SPI_ChangeCS(port, 1);            // INactive!
    return Temp;
}
void SPI_Flash_Read(uint32_t port, UCHAR *pBuffer, UINT32 ReadAddr, uint16_t size)
{
    uint32_t i;
    uint8_t cmd = W25X_ReadData;
    uint8_t addr = 0;
    uint8_t data;

    CH347SPI_ChangeCS(port, 0);
    cmd = W25X_ReadData;
    //SPI1_ReadWriteByte(W25X_ReadData);
    CH347SPI_WriteRead(port, 0, 1, &cmd);
    //SPI1_ReadWriteByte((u8)((ReadAddr) >> 16));
    addr = ((ReadAddr) >> 16);
    CH347SPI_WriteRead(port, 0, 1, &addr);
    //SPI1_ReadWriteByte((u8)((ReadAddr) >> 8));
    addr = ((ReadAddr) >> 8);
    CH347SPI_WriteRead(port, 0, 1, &addr);
    //SPI1_ReadWriteByte((u8)ReadAddr);
    addr = ((ReadAddr) & 0xFF);
    CH347SPI_WriteRead(port, 0, 1, &addr);

    for (i = 0; i < size; i++)
    {
        data = 0xFF;
        CH347SPI_WriteRead(port, 0, 1, &data);
        pBuffer[i] = data;
    }
    CH347SPI_ChangeCS(port, 0);
}

int main() {
    UCHAR drv;
    UCHAR dll;
    UCHAR bcd;
    UCHAR chip;
    UCHAR bser[40];
    mSpiCfgS spi_cfg;
    mDeviceInforS dev_info;
    UCHAR buff[1024 * 40] = { 0 };
    ULONG bsz = 1024;
    UINT16 Flash_Model, Jedec;


    std::cout << "*1*" << std::endl;

    HANDLE hh = CH347OpenDevice(0);

    if (hh) {
        CH347GetVersion(0, &drv, &dll, &bcd, &chip);
        printf("Found driver CH3_%d, dll ver%d, bcd %d and chip type %d\r\n", drv, dll, bcd, chip);
        CH347GetSerialNumber(0, bser);
        printf("Serial number is %s\r\n", bser);
        CH347GPIO_Get(0, &drv, &bcd);
        printf("GPIO dir: %x, value: %x\r\n", drv, bcd);
        CH347GetDeviceInfor(0, &dev_info);
        CH347SPI_GetCfg(0, &spi_cfg);
        printf("IfNum: %d\r\n", dev_info.CH347IfNum);

        spi_cfg.iMode = 0;
        spi_cfg.CS1Polarity = 0;
        spi_cfg.CS2Polarity = 0;
        spi_cfg.iChipSelect = 0x80; //valid, CS1 is working
        spi_cfg.iClock = 2;     // 3.75 MHz
        spi_cfg.iByteOrder = 1; // MSB First

        CH347SPI_SetFrequency(0, 1500000);
        BOOL r = CH347SPI_Init(0, &spi_cfg);
        if(r) printf("Init is done!\r\n");
        else printf("Init error!\r\n");
        CH347SPI_ChangeCS(0, 1);            // inactive!
        printf("CS set to HIGH\r\n");
        CH347SPI_ChangeCS(0, 0);            // active!
        printf("CS set to LOW\r\n");
        CH347SPI_SetChipSelect(0, 0x101, 1, 0, 0, 0);   // переключает обратно в inactive


        Flash_Model = SPI_Flash_ReadID(0);
        printf("Flash_Model is %04X\r\n", Flash_Model);
        Jedec = SPI_Flash_ReadJEDEC(0);
        printf("JEDEC ID is %04X\r\n", Jedec);
        memset(buff, 0x55, 1024 * 40);
        SPI_Flash_Read(0, buff, 0, 1024 * 40);
/*
    CH347SPI_SetChipSelect(ULONG iIndex,           // Specify device number
                                   USHORT iEnableSelect,   // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
                                   USHORT iChipSelect,     // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
                                   ULONG iIsAutoDeativeCS, // The lower 16 bits are CS1 and the higher 16 bits are CS2. Whether to undo slice selection automatically after the operation is complete
                                   ULONG iActiveDelay,     // The lower 16 bits are CS1 and the higher 16 bits are CS2. Set the latency of read/write operations after chip selection, the unit is us
                                   ULONG iDelayDeactive);  // The lower 16 bits are CS1 and the higher 16 bits are CS2. Delay time for read and write operations after slice selection the unit is us
*/

        // Read USB data (не подходит, читает 0 байт)
        //CH347ReadData(0, buff, &bsz);
        CH347CloseDevice(0);
    }
    /*
    test_universal_ctor();
    test_virtual_methods();
    test_template_members();
    test_compile_time_conditional();
    test_template_cont();
    test_template_cont_func();
    test_template_static_methods();
    */
    return 0;
}