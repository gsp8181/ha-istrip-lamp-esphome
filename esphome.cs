using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.Advertisement;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Storage.Streams;
using System.Security.Cryptography;

public class BluetoothService
{
    //adapted from com.ben.istrips
    private BluetoothLEAdvertisementWatcher _watcher;
    private readonly Random _random = new Random();
    private readonly byte[] _targetManufacturerData = new byte[] { 0x00, 0x57, 0x00, 0x00, 0x53 };
    private readonly ushort _targetVendorId = 0x5254;
    private readonly Guid _targetCharacteristicUuid = new Guid("0000ac52-1212-efde-1523-785fedbeda25");
    private GattCharacteristic _targetCharacteristic; // new field to store the connected characteristic
    //Dump converter
   // https://gchq.github.io/CyberChef/#recipe=From_Decimal('Space',false/disabled)To_Hex('Space',0/disabled)AES_Decrypt(%7B'option':'Hex','string':'34-52-2A-5B-7A-6E-49-2C-08-09-0A-9D-8D-2A-23-F8'%7D,%7B'option':'Hex','string':''%7D,'ECB','Hex','Raw',%7B'option':'Hex','string':''%7D,%7B'option':'Hex','string':''%7D)To_Hexdump(16,false,false,false)&input=ZjEgZTUgN2IgY2UgMmYgNTggMTcgOTAgZmEgYTggMGEgZjUgY2UgZTQgMmQgZjgNCg&ieol=CRLF
    private readonly byte[] _encryptionKey = new byte[] { 
        52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248 
    };

    public BluetoothService()
    {
        _watcher = new BluetoothLEAdvertisementWatcher();
        _watcher.SignalStrengthFilter.SamplingInterval = TimeSpan.FromMilliseconds(100);
    }

    public async Task StartScanningAsync()
    {
        _watcher.Received += Watcher_Received;
        _watcher.Start();
    }

    private async void Watcher_Received(BluetoothLEAdvertisementWatcher sender, BluetoothLEAdvertisementReceivedEventArgs args)
    {
        var manufacturerData = args.Advertisement.ManufacturerData;
        
        if (!manufacturerData.Any()) return;
        
        var data = manufacturerData.First();
        string localName = args.Advertisement.LocalName;

        var dataValue = new byte[data.Data.Length];
        using (var reader = DataReader.FromBuffer(data.Data))
        {
            reader.ReadBytes(dataValue);
        }

        if (data.CompanyId != _targetVendorId)
        {
            //Console.WriteLine($"Not matching vendor ID 0x{_targetVendorId:X4}, skipping device");
            return;
        }

        Console.WriteLine($"Found device - Name: {(string.IsNullOrEmpty(localName) ? "Unknown" : localName)}");
        Console.WriteLine($"Address: {args.BluetoothAddress:X12}, Company ID: 0x{data.CompanyId:X4}");
        Console.WriteLine($"Services: {string.Join(", ", args.Advertisement.ServiceUuids.Select(uuid => uuid.ToString()))}");
        Console.WriteLine($"Manufacturer Data: {BitConverter.ToString(dataValue)}");

        try
        {
            var device = await BluetoothLEDevice.FromBluetoothAddressAsync(args.BluetoothAddress);
            Console.WriteLine($"\nConnecting to device: {device.Name} ({args.BluetoothAddress:X12})");
            
            var result = await device.GetGattServicesAsync(BluetoothCacheMode.Uncached);
            if (result.Status != GattCommunicationStatus.Success)
            {
                Console.WriteLine($"Failed to get services: {result.Status}");
                return;
            }

            foreach (var service in result.Services)
            {
                var characteristicsResult = await service.GetCharacteristicsAsync(BluetoothCacheMode.Uncached);
                if (characteristicsResult.Status != GattCommunicationStatus.Success)
                {
                    continue;
                }

                var targetCharacteristic = characteristicsResult.Characteristics
                    .FirstOrDefault(c => c.Uuid == _targetCharacteristicUuid);
                if (targetCharacteristic != null)
                {
                    Console.WriteLine($"Found target characteristic: {targetCharacteristic.Uuid}");
                    _targetCharacteristic = targetCharacteristic; // store for subsequent use
                    //await SendCommandAsync(targetCharacteristic, _commandData);
                    await SendColorCommand(1, 1, 0x00, 0x00, 0xff, 64, 0, 20);
                    break;
                }
            }

            // Cleanup
            foreach (var service in result.Services)
            {
                service.Dispose();
            }
            device.Dispose();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error processing device: {ex.Message}");
        }
    }

    private byte[] EncryptData(byte[] data)
    {
        // Pad data to 16 byte blocks if needed
        int padding = 16 - (data.Length % 16);
        byte[] paddedData = new byte[data.Length + (padding == 16 ? 0 : padding)];
        Array.Copy(data, paddedData, data.Length);
        
        if (padding != 16)
        {
            for (int i = data.Length; i < paddedData.Length; i++)
            {
                paddedData[i] = (byte)padding;
            }
        }

        using (Aes aes = Aes.Create())
        {
            aes.Key = _encryptionKey;
            aes.Mode = CipherMode.ECB;
            aes.Padding = PaddingMode.None;

            using (ICryptoTransform encryptor = aes.CreateEncryptor())
            {
                return encryptor.TransformFinalBlock(paddedData, 0, paddedData.Length);
            }
        }
    }

    private async Task SendCommandAsync(GattCharacteristic characteristic, byte[] data)
    {
        try
        {
            var encryptedData = EncryptData(data);
            Console.WriteLine($"  Original data: {BitConverter.ToString(data)}");
            Console.WriteLine($"  Encrypted data: {BitConverter.ToString(encryptedData)}");

            var writer = new DataWriter();
            writer.WriteBytes(encryptedData);
            var result = await characteristic.WriteValueAsync(writer.DetachBuffer());
            
            if (result == GattCommunicationStatus.Success)
            {
                Console.WriteLine($"  Command sent successfully to {characteristic.Uuid}");
            }
            else
            {
                Console.WriteLine($"  Failed to write to {characteristic.Uuid}: {result}");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"  Error sending command to {characteristic.Uuid}: {ex.Message}");
        }
    }

    // New methods accepting explicit parameters instead of DataManager
    
    // Sends a color command:  
    //max speed 64
    //max light 64
    // Format: {84,82,0,87, 2, groupId, extraParam, red, green, blue, light, speed, type, 0,0,0}
    //Extra params, 1=Fade7, 2=Fade3, 3=Breath R, 4=Strobe R, 5=Breath7, 6=Breath3, 7=Breath B, 8=Strobe B, 9=Flash7, 10=Flash3, 11=Breath G, 12=Strobe G
    public async Task SendColorCommand(byte groupId, byte extraParam, byte red, byte green, byte blue, byte light, byte speed, byte type)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 2, groupId, extraParam, red, green, blue, light, speed, type, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends a LED off command:
    // Format: {84,82,0,87, 2, groupId, 0,0,0,0, light, speed, 0,0,0,0}
    public async Task SendLedOffCommand(byte groupId, byte light, byte speed)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 2, groupId, 0, 0, 0, 0, light, speed, 0, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends a rhythm command:
    // Format: {84,82,0,87, 3, groupId, fftStatus, fft, sensitivity, 0,0,0,0,0,0,0}
    public async Task SendRhythmCommand(byte groupId, byte fftStatus, byte fft, byte sensitivity)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 3, groupId, fftStatus, fft, sensitivity, 0, 0, 0, 0, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends a timer command:
    // Format: {84,82,0,87, 4, groupId, week, currentHour, currentMinute, currentSecond, timerOnWeek, timerOnHour, timerOnMin, timerOffWeek, timerOffHour, timerOffMin}
    public async Task SendTimerCommand(byte groupId, byte week, byte currentHour, byte currentMinute, byte currentSecond,
                                       byte timerOnWeek, byte timerOnHour, byte timerOnMin,
                                       byte timerOffWeek, byte timerOffHour, byte timerOffMin)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 4, groupId, week, currentHour, currentMinute, currentSecond, timerOnWeek, timerOnHour, timerOnMin, timerOffWeek, timerOffHour, timerOffMin};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends an RGB line sequence command:
    // Format: {84,82,0,87, 5, groupId, extraParam, 0,0,0,0,0,0,0,0,0}
    public async Task SendRGBLineSequenceCommand(byte groupId, byte extraParam)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 5, groupId, extraParam, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends a speed command:
    // Format: {84,82,0,87, 6, groupId, speed, 0,0,0,0,0,0,0,0,0}
    public async Task SendSpeedCommand(byte groupId, byte speed)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 6, groupId, speed, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }

    // Sends a light command:
    // Format: {84,82,0,87, 7, groupId, light, 0,0,0,0,0,0,0,0,0}
    public async Task SendLightCommand(byte groupId, byte light)
    {
        if (_targetCharacteristic == null)
        {
            Console.WriteLine("Characteristic not connected.");
            return;
        }
        byte[] command = {84,82,0,87, 7, groupId, light, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        await SendCommandAsync(_targetCharacteristic, command);
    }
}
