/**
 * @file singleton.hpp
 * @brief 单例模式模板类实现
 * @details 提供线程安全的单例模式基类模板，支持延迟初始化，
 *          通过getInstance()方法获取唯一实例，
 *          禁止拷贝构造和拷贝赋值操作
 * @author Louis
 * @date 2025-10-3
 * @version 1.0.0
 */

#pragma once

namespace louis {
    namespace patterns {
        template<class T>
        class Singleton {
        public:
            // 提供一个公有的获取实例的静态方法
            static T* getInstance() {
                if (m_instance == nullptr) {
                    m_instance = new T();
                }
                return m_instance;
            }

            // 拷贝构造和拷贝赋值运算符声明为 delete 时，建议放在 public 下
            Singleton(const Singleton &) = delete;
            Singleton &operator=(const Singleton &) = delete;
        protected:
            // 将类的构造函数设为私有方法，禁止实例化
            // 设置为protected，允许派生类析构调用，同时禁止外部构造和析构
            Singleton() = default;
            ~Singleton() = default; // 设为默认方法（编译器生成）

            // 定义一个私有的静态成员变量，用于保存单例对象
            static T* m_instance;
        };

        // 静态成员变量类外初始化
        template<typename T>
        T* Singleton<T>::m_instance = nullptr;
    } // namespace patterns
} // namespace louis

