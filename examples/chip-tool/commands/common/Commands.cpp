/*
 *   Copyright (c) 2020 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include "Commands.h"

#include "Command.h"
#include "Logging.h"

#include <algorithm>
#include <string>

#include <support/CHIPMem.h>

void Commands::Register(const char * clusterName, commands_list commandsList)
{
    for (auto & command : commandsList)
    {
        mClusters[clusterName].push_back(std::move(command));
    }
}

int Commands::Run(NodeId localId, NodeId remoteId, int argc, char ** argv)
{
    ConfigureChipLogging();

    CHIP_ERROR err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Controller, "Init Memory failure: %s", chip::ErrorStr(err));
    }
    else
    {

        ChipDeviceController dc;

        err = dc.Init(localId);
        VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(Controller, "Init failure: %s", chip::ErrorStr(err)));

        err = dc.ServiceEvents();
        VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(Controller, "Init Run Loop failure: %s", chip::ErrorStr(err)));

        err = RunCommand(dc, remoteId, argc, argv);
        SuccessOrExit(err);

    exit:
        dc.ServiceEventSignal();
        dc.Shutdown();
    }
    return (err == CHIP_NO_ERROR) ? EXIT_SUCCESS : EXIT_FAILURE;
}

CHIP_ERROR Commands::RunCommand(ChipDeviceController & dc, NodeId remoteId, int argc, char ** argv)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    std::map<std::string, CommandsVector>::iterator cluster;
    Command * command = nullptr;

    if (argc <= 1)
    {
        ChipLogError(chipTool, "Missing cluster name");
        ShowClusters(argv[0]);
        ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
    }

    cluster = GetCluster(argv[1]);
    if (cluster == mClusters.end())
    {
        ChipLogError(chipTool, "Unknown cluster: %s", argv[1]);
        ShowClusters(argv[0]);
        ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
    }

    if (argc <= 2)
    {
        ChipLogError(chipTool, "Missing command name");
        ShowCluster(argv[0], argv[1], cluster->second);
        ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
    }

    if (strcmp(argv[2], "read") != 0 && strcmp(argv[2], "write") != 0)
    {
        command = GetCommand(cluster->second, argv[2]);
        if (command == nullptr)
        {
            ChipLogError(chipTool, "Unknown command: %s", argv[2]);
            ShowCluster(argv[0], argv[1], cluster->second);
            ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
        }
    }
    else
    {
        if (argc <= 3)
        {
            ChipLogError(chipTool, "Missing attribute name");
            ShowClusterAttributes(argv[0], argv[1], argv[2], cluster->second);
            ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
        }

        command = GetReadWriteCommand(cluster->second, argv[2], argv[3]);
        if (command == nullptr)
        {
            ChipLogError(chipTool, "Unknown attribute: %s", argv[3]);
            ShowClusterAttributes(argv[0], argv[1], argv[2], cluster->second);
            ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
        }
    }

    if (!command->InitArguments(argc - 3, &argv[3]))
    {
        ShowCommand(argv[0], argv[1], command);
        ExitNow(err = CHIP_ERROR_INVALID_ARGUMENT);
    }

    err = command->Run(&dc, remoteId);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(chipTool, "Run command failure: %s", chip::ErrorStr(err));
        ExitNow();
    }

exit:
    return err;
}

std::map<std::string, Commands::CommandsVector>::iterator Commands::GetCluster(std::string clusterName)
{
    for (auto & cluster : mClusters)
    {
        std::string key(cluster.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (key.compare(clusterName) == 0)
        {
            return mClusters.find(cluster.first);
        }
    }

    return mClusters.end();
}

Command * Commands::GetCommand(CommandsVector & commands, std::string commandName)
{
    for (auto & command : commands)
    {
        if (commandName.compare(command->GetName()) == 0)
        {
            return command.get();
        }
    }

    return nullptr;
}

Command * Commands::GetReadWriteCommand(CommandsVector & commands, std::string commandName, std::string attributeName)
{
    for (auto & command : commands)
    {
        if (commandName.compare(command->GetName()) == 0 && attributeName.compare(command->GetAttribute()) == 0)
        {
            return command.get();
        }
    }

    return nullptr;
}

void Commands::ShowClusters(std::string executable)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s cluster_name command_name [param1 param2 ...]\n", executable.c_str());
    fprintf(stderr, "\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    fprintf(stderr, "  | Clusters:                                                                           |\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    for (auto & cluster : mClusters)
    {
        fprintf(stderr, "  | * %-82s|\n", cluster.first.c_str());
    }
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
}

void Commands::ShowCluster(std::string executable, std::string clusterName, CommandsVector & commands)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s %s command_name [param1 param2 ...]\n", executable.c_str(), clusterName.c_str());
    fprintf(stderr, "\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    fprintf(stderr, "  | Commands:                                                                           |\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    bool readCommand  = false;
    bool writeCommand = false;
    for (auto & command : commands)
    {
        if (strcmp(command->GetName(), "read") == 0)
        {
            if (readCommand == false)
            {
                fprintf(stderr, "  | * %-82s|\n", command->GetName());
                readCommand = true;
            }
        }
        else if (strcmp(command->GetName(), "write") == 0)
        {
            if (writeCommand == false)
            {
                fprintf(stderr, "  | * %-82s|\n", command->GetName());
                writeCommand = true;
            }
        }
        else
        {
            fprintf(stderr, "  | * %-82s|\n", command->GetName());
        }
    }
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
}

void Commands::ShowClusterAttributes(std::string executable, std::string clusterName, std::string commandName,
                                     CommandsVector & commands)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s %s %s attribute-name [param1 param2 ...]\n", executable.c_str(), clusterName.c_str(),
            commandName.c_str());
    fprintf(stderr, "\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    fprintf(stderr, "  | Attributes:                                                                         |\n");
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
    for (auto & command : commands)
    {
        if (commandName.compare(command->GetName()) == 0)
        {
            fprintf(stderr, "  | * %-82s|\n", command->GetAttribute());
        }
    }
    fprintf(stderr, "  +-------------------------------------------------------------------------------------+\n");
}

void Commands::ShowCommand(std::string executable, std::string clusterName, Command * command)
{
    fprintf(stderr, "Usage:\n");

    std::string arguments = "";
    arguments += command->GetName();

    size_t argumentsCount = command->GetArgumentsCount();
    for (size_t i = 0; i < argumentsCount; i++)
    {
        arguments += " ";
        arguments += command->GetArgumentName(i);
    }
    fprintf(stderr, "  %s %s %s\n", executable.c_str(), clusterName.c_str(), arguments.c_str());
}
