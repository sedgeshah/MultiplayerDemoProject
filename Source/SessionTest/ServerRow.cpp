// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerRow.h"
#include "Components/TextBlock.h"

void UServerRow::NativeConstruct()
{
	Super::NativeConstruct();
}

void UServerRow::SetServerName(FString arg_ServerName)
{
	ServerName->SetText(FText::FromString(arg_ServerName));
}

FText UServerRow::GetServerName()
{
	return ServerName->GetText();
}

void UServerRow::SetServerID(FString arg_ServerID)
{
	ServerID->SetText(FText::FromString(arg_ServerID));
}

FText UServerRow::GetServerID()
{
	return ServerID->GetText();
}