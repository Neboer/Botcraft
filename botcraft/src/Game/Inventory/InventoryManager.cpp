#include "botcraft/Game/Inventory/InventoryManager.hpp"
#include "botcraft/Game/Inventory/Inventory.hpp"

using namespace ProtocolCraft;

namespace Botcraft
{
    InventoryManager::InventoryManager()
    {
        index_hotbar_selected = 0;
        cursor = Slot();
    }

    std::mutex& InventoryManager::GetMutex()
    {
        return inventory_manager_mutex;
    }

    void InventoryManager::SetSlot(const short window_id, const short index, const Slot &slot)
    {
        auto it = inventories.find(window_id);

        if (it == inventories.end())
        {
            inventories[window_id] = std::shared_ptr<Inventory>(new Inventory);
            inventories[window_id]->GetSlots()[index] = slot;
        }
        else
        {
            it->second->GetSlots()[index] = slot;
        }
    }

    std::shared_ptr<Inventory> InventoryManager::GetInventory(const short window_id)
    {
        auto it = inventories.find(window_id);
        if (it == inventories.end())
        {
            return nullptr;
        }
        return it->second;
    }
    
    std::shared_ptr<Inventory> InventoryManager::GetPlayerInventory()
    {
        return GetInventory(Inventory::PLAYER_INVENTORY_INDEX);
    }
    
    const std::shared_ptr<Inventory> InventoryManager::GetInventory(const short window_id) const
    {
        auto it = inventories.find(window_id);
        if (it == inventories.end())
        {
            return nullptr;
        }
        return it->second;
    }

    const std::shared_ptr<Inventory> InventoryManager::GetPlayerInventory() const
    {
        return GetInventory(Inventory::PLAYER_INVENTORY_INDEX);
    }
    
    const Slot InventoryManager::GetHotbarSelected() const
    {
        const std::shared_ptr<Inventory> inventory = GetPlayerInventory();

        if (!inventory)
        {
            return Slot();
        }

        inventory->GetSlot(Inventory::INVENTORY_HOTBAR_START + index_hotbar_selected);
    }

    void InventoryManager::EraseInventory(const short window_id)
    {
        inventories.erase(window_id);
    }

    void InventoryManager::AddInventory(const short window_id)
    {
        inventories[window_id] = std::shared_ptr<Inventory>(new Inventory);
    }

    void InventoryManager::SetHotbarSelected(const short index)
    {
        index_hotbar_selected = index;
    }

    const Slot& InventoryManager::GetCursor() const
    {
        return cursor;
    }

    void InventoryManager::SetCursor(const Slot &c)
    {
        cursor = c;
    }

    void InventoryManager::Handle(ProtocolCraft::Message& msg)
    {

    }

    void InventoryManager::Handle(ProtocolCraft::SetSlot& msg)
    {
        std::lock_guard<std::mutex> inventories_locker(inventory_manager_mutex);

        if (msg.GetWindowId() == -1 && msg.GetSlot() == -1)
        {
            SetCursor(msg.GetSlotData());
        }
        else if (msg.GetWindowId() == -2)
        {
            SetSlot(Inventory::PLAYER_INVENTORY_INDEX, msg.GetSlot(), msg.GetSlotData());
        }
        else if (msg.GetWindowId() >= 0)
        {
            SetSlot(msg.GetWindowId(), msg.GetSlot(), msg.GetSlotData());
        }
        else
        {
            std::cerr << "Warning, unknown window called in SetSlot: " << msg.GetWindowId() << ", " << msg.GetSlot() << std::endl;
        }
    }

    void InventoryManager::Handle(ProtocolCraft::WindowItems& msg)
    {
        std::lock_guard<std::mutex> inventory_manager_locker(inventory_manager_mutex);
        for (int i = 0; i < msg.GetCount(); ++i)
        {
            SetSlot(msg.GetWindowId(), i, msg.GetSlotData()[i]);
        }
    }

    void InventoryManager::Handle(ProtocolCraft::OpenWindow& msg)
    {
        std::lock_guard<std::mutex> inventory_manager_locker(inventory_manager_mutex);
        AddInventory(msg.GetWindowId());
    }

    void InventoryManager::Handle(ProtocolCraft::HeldItemChangeClientbound& msg)
    {
        std::lock_guard<std::mutex> inventory_manager_locker(inventory_manager_mutex);
        SetHotbarSelected(msg.GetSlot());
    }


} //Botcraft