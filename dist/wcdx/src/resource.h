#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED
#pragma once

//#include <stdext/traits.h>

#include <functional>
#include <type_traits>
#include <utility>


template <class Handle>
class SmartResource
{
public:
    using Deleter = std::function<void (Handle)>;

public:
    template <class H = Handle, std::enable_if_t<(std::is_default_constructible_v<H>), std::nullptr_t> = nullptr>
    SmartResource()
        : handle(), deleter()
    {
    }

    template <class H,
        std::enable_if_t<(std::is_constructible_v<Handle, decltype(std::forward<H>(std::declval<H&&>()))>), std::nullptr_t> = nullptr>
    explicit SmartResource(H&& handle, Deleter deleter)
        : handle(std::forward<H>(handle)), deleter(std::move(deleter))
    {
    }

    SmartResource(const SmartResource&) = delete;
    SmartResource& operator = (const SmartResource&) = delete;

    SmartResource(SmartResource&& other)
        : handle(std::move(other.handle)), deleter(std::move(other.deleter))
    {
    }

    SmartResource& operator = (SmartResource&& other)
    {
        if (deleter)
            deleter(handle);

        handle = std::move(other.handle);
        deleter = std::move(other.deleter);
        return *this;
    }

    ~SmartResource()
    {
        if (deleter != nullptr)
            deleter(handle);
    }

public:
    operator Handle () const { return handle; }
    Handle Get() const { return handle; }

    template <class H,
    std::enable_if_t<(std::is_assignable_v<Handle&, decltype(std::forward<H>(std::declval<H&&>()))>), std::nullptr_t> = nullptr>
    void Reset(H&& handle, Deleter deleter)
    {
        if (this->deleter)
            this->deleter(this->handle);

        this->handle = std::forward<H>(handle);
        this->deleter = std::move(deleter);
    }

    void Invalidate() { deleter = {}; }

private:
    Handle handle;
    Deleter deleter;
};

#endif
