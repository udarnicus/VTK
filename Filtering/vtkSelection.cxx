/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelection.h"

#include "vtkAbstractArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
struct vtkSelectionInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> > Nodes;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSelection, "1.28");
vtkStandardNewMacro(vtkSelection);

//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
{
  this->Internal = new vtkSelectionInternals;
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkSelection::~vtkSelection()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSelection::Initialize()
{
  this->Superclass::Initialize();
  delete this->Internal;
  this->Internal = new vtkSelectionInternals;
}

//----------------------------------------------------------------------------
unsigned int vtkSelection::GetNumberOfNodes()
{
  return static_cast<unsigned int>(this->Internal->Nodes.size());
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelection::GetNode(unsigned int idx)
{
  if (idx >= this->GetNumberOfNodes())
    {
    return 0;
    }
  return this->Internal->Nodes[idx];
}

//----------------------------------------------------------------------------
void vtkSelection::AddNode(vtkSelectionNode* node)
{
  if (!node)
    {
    return;
    }
  // Make sure that node is not already added
  unsigned int numNodes = this->GetNumberOfNodes();
  for (unsigned int i = 0; i < numNodes; i++)
    {
    if (this->GetNode(i) == node)
      {
      return;
      }
    }
  this->Internal->Nodes.push_back(node);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(unsigned int idx)
{
  if (idx >= this->GetNumberOfNodes())
    {
    return;
    }
  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> >::iterator iter =
    this->Internal->Nodes.begin();
  this->Internal->Nodes.erase(iter+idx);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(vtkSelectionNode* node)
{
  if (!node)
    {
    return;
    }
  unsigned int numNodes = this->GetNumberOfNodes();
  for (unsigned int i = 0; i < numNodes; i++)
    {
    if (this->GetNode(i) == node)
      {
      this->RemoveNode(i);
      return;
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveAllNodes()
{
  this->Internal->Nodes.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  unsigned int numNodes = this->GetNumberOfNodes();
  os << indent << "Number of nodes: " << numNodes << endl;
  os << indent << "Nodes: " << endl;
  for (unsigned int i = 0; i < numNodes; i++)
    {
    os << indent << "Node #" << i << endl;
    this->GetNode(i)->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkSelection::ShallowCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
    {
    return;
    }
  this->Initialize();
  this->Superclass::ShallowCopy(src);
  unsigned int numNodes = input->GetNumberOfNodes();
  for (unsigned int i=0; i<numNodes; i++)
    {
    vtkSmartPointer<vtkSelectionNode> newNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    newNode->ShallowCopy(input->GetNode(i));
    this->AddNode(newNode);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::DeepCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
    {
    return;
    }
  this->Initialize();
  this->Superclass::DeepCopy(src);
  unsigned int numNodes = input->GetNumberOfNodes();
  for (unsigned int i=0; i<numNodes; i++)
    {
    vtkSmartPointer<vtkSelectionNode> newNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    newNode->DeepCopy(input->GetNode(i));
    this->AddNode(newNode);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelection* s)
{
  for (unsigned int n = 0; n < s->GetNumberOfNodes(); ++n)
    {
    this->Union(s->GetNode(n));
    }
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelectionNode* node)
{
  bool merged = false;
  for (unsigned int tn = 0; tn < this->GetNumberOfNodes(); ++tn)
    {
    vtkSelectionNode* tnode = this->GetNode(tn);
    if (tnode->EqualProperties(node))
      {
      tnode->UnionSelectionList(node);
      merged = true;
      break;
      }
    }
  if (!merged)
    {
    vtkSmartPointer<vtkSelectionNode> clone =
      vtkSmartPointer<vtkSelectionNode>::New();
    clone->ShallowCopy(node);
    this->AddNode(clone);
    }
}

//----------------------------------------------------------------------------
unsigned long vtkSelection::GetMTime()
{
  unsigned long mTime = this->MTime.GetMTime();
  unsigned long nodeMTime;
  for (unsigned int n = 0; n < this->GetNumberOfNodes(); ++n)
    {
    vtkSelectionNode* node = this->GetNode(n);
    nodeMTime = node->GetMTime();
    mTime = ( nodeMTime > mTime ? nodeMTime : mTime );
    }
  return mTime;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformation* info)
{
  return info? vtkSelection::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformationVector* v, int i)
{
  return vtkSelection::GetData(v->GetInformationObject(i));
}
